![](./pic/intel_logo.png)

# **SDK Developer Reference for VP8**
## Media SDK API Version 1.30

<div style="page-break-before:always" />

[**LEGAL DISCLAIMER**](./header-template.md#legal-disclaimer)

[**Optimization Notice**](./header-template.md#optimization-notice)

<div style="page-break-before:always" />

- [Overview](#overview)
  * [Document Conventions](#document-conventions)
  * [Acronyms and Abbreviations](#acronyms-and-abbreviations)
- [Architecture & Programming Guide](#architecture---programming-guide)
  * [Decoding Procedure](#decoding-procedure)
  * [Encoding Procedure](#encoding-procedure)
- [Structure Reference](#structure-reference)
  * [mfxExtVP8CodingOption](#mfxExtCodingOptionVP8)
- [Enumerator Reference](#enumerator-reference)
  * [CodecProfile](#CodecProfile)
  * [ExtendedBufferID](#ExtendedBufferID)

# Overview

Intel® Media Software Development Kit – SDK is a software development library that exposes the media acceleration capabilities of Intel platforms for decoding, encoding and video processing. The API library covers a wide range of Intel platforms.

This document describes the SDK extension to support VP8 video codec.

## Document Conventions

The SDK API uses the Verdana typeface for normal prose. With the exception of section headings and the table of contents, all code-related items appear in the `Courier New` typeface. Examples relevant to this document are `mfxStatus` and `MFXInit`. Hyperlinks appear in underlined boldface,
such as `mfxStatus`.

## Acronyms and Abbreviations

| | |
--- | ---
**SDK** | Intel® Media Software Development Kit – SDK
`API` | Application Programming Interface

# Architecture & Programming Guide

SDK extension for VP8 requires the application to use an additional include file `mfxvp8.h`, in addition to the regular SDK include files. No additional library is needed at the link time.

```C
Include these files:
#include “mfxvideo.h”    /* SDK functions in C */
#include “mfxvideo++.h”    /* Optional for C++ development */
#include “mfxvp8.h”        /* VP8 development */
Link to this library:
    libmfx.lib            /* The SDK dispatcher library */
```

The SDK extends the codec identifier `MFX_CODEC_VP8` for VP8 processing.

## Decoding Procedure

The application should use the same decoding procedure that described in the [*SDK API Reference Manual*](./mediasdk-man.md). The only difference is in partitioning of input bitstream. Unlike other supported by SDK decoders, VP8 can accept only complete frame as input and application should provide it accompanied by `MFX_BITSTREAM_COMPLETE_FRAME` flag.

## Encoding Procedure

The application should use the same encoding procedure that described in the [*SDK API Reference Manual*](./mediasdk-man.md).

# Structure Reference

## <a id='mfxExtCodingOptionVP8'>mfxExtVP8CodingOption</a>

**Definition**

```C
typedef struct {
    mfxExtBuffer    Header;

    mfxU16   Version;
    mfxU16   EnableMultipleSegments;
    mfxU16   LoopFilterType;
    mfxU16   LoopFilterLevel[4];
    mfxU16   SharpnessLevel;
    mfxU16   NumTokenPartitions;
    mfxI16   LoopFilterRefTypeDelta[4];
    mfxI16   LoopFilterMbModeDelta[4];
    mfxI16   SegmentQPDelta[4];
    mfxI16   CoeffTypeQPDelta[5];
    mfxU16   WriteIVFHeaders;
    mfxU32   NumFramesForIVFHeader;
    mfxU16   reserved[223];
} mfxExtVP8CodingOption;
```

**Description**

This `mfxExtVP8CodingOption` structure describes VP8 encoder configuration parameters.

**Members**

| | |
--- | ---
`Header.BufferId` | Must be set to `MFX_EXTBUFF_VP8_CODING_OPTION.`
`Version` | Determines the bitstream version. Corresponds to the same VP8 syntax element in `frame_tag`.
`EnableMultipleSegments` | Set this option to ON, to enable segmentation. This is tri-state option. See the `CodingOptionValue` enumerator for values of this option in the [*SDK API Reference Manual*](./mediasdk-man.md) for details
`LoopFilterType` | Selecting the type of filter (normal or simple). Corresponds to VP8 syntax element `filter_type`.
`LoopFilterLevel` | Controls the filter strength. Corresponds to VP8 syntax element `loop_filter_level`.
`SharpnessLevel` | Controls the filter sensitivity. Corresponds to VP8 syntax element `sharpness_level`.
`NumTokenPartitions` | Specifies number of token partitions in the coded frame.
`LoopFilterRefTypeDelta` | Loop filter level delta for reference type (intra, last, golden, altref).
`LoopFilterMbModeDelta` | Loop filter level delta for MB modes.
`SegmentQPDelta` | QP delta for segment.
`CoeffTypeQPDelta` | QP delta for coefficient type (YDC, Y2AC, Y2DC, UVAC, UVDC).
`WriteIVFHeaders` | Set this option to ON, to enable insertion of IVF container headers into bitstream. This is tri-state option. See the `CodingOptionValue` enumerator for values of this option in the [*SDK API Reference Manual*](./mediasdk-man.md) for details
`NumFramesForIVFHeader` | Specifies number of frames for IVF header when `WriteIVFHeaders` is ON.

**Change History**

This structure is available since SDK API 1.12.

# Enumerator Reference

## <a id='CodecProfile'>CodecProfile</a>

**Description**

The `CodecProfile` enumerator is extended to support VP8 profiles. See the [*SDK API Reference Manual*](./mediasdk-man.md) for additional profile definitions.

**Name/Description**

| | |
--- | ---
`MFX_PROFILE_VP8_0`, `MFX_PROFILE_VP8_1`, `MFX_PROFILE_VP8_2`, `MFX_PROFILE_VP8_3` | VP8 profiles

**Change History**

This enumerator is available since SDK API 1.0. SDK API 1.12 added VP8 profiles.

## <a id='ExtendedBufferID'>ExtendedBufferID</a>

**Description**

The `ExtendedBufferID` enumerator is extended to add VP8 support. See the [*SDK API Reference Manual*](./mediasdk-man.md) for additional definitions.

**Name/Description**

| | |
--- | ---
`MFX_EXTBUFF_VP8_CODING_OPTION` | This extended buffer describes VP8 encoder configuration parameters. See the [mfxExtVP8CodingOption](#mfxExtCodingOptionVP8) structure for details. The application can attach this buffer to the `mfxVideoParam` structure for encoding initialization.

**Change History**

This enumerator is available since SDK API 1.0.

SDK API 1.12 adds `MFX_EXTBUFF_VP8_CODING_OPTION`.
