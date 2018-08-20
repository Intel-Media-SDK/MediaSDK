// Copyright (c) 2017 Intel Corporation
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef __JPEGBASE_H__
#define __JPEGBASE_H__

#include "umc_defs.h"
#if defined (MFX_ENABLE_MJPEG_VIDEO_DECODE) || defined (MFX_ENABLE_MJPEG_VIDEO_ENCODE)
#include "umc_defs.h"
#include "vm_strings.h"

#ifdef _DEBUG
#define ENABLE_TRACING
#endif
#undef ENABLE_ERROR_LOGGING
//#define ENABLE_ERROR_LOGGING

#define DIB_ALIGN (sizeof(int) - 1)

#define DIB_UWIDTH(width,nchannels) \
  ((width) * (nchannels))

#define DIB_AWIDTH(width,nchannels) \
  ( ((DIB_UWIDTH(width,nchannels) + DIB_ALIGN) & (~DIB_ALIGN)) )

#define DIB_PAD_BYTES(width,nchannels) \
  ( DIB_AWIDTH(width,nchannels) - DIB_UWIDTH(width,nchannels) )



#undef ENABLE_TRACING
#ifdef ENABLE_TRACING

#define TRC(msg) \
  cout << (msg)

#define TRC0(msg) \
  cout << (msg) << endl

#define TRC1(msg,p1) \
  cout << (msg) << (p1) << endl

#else

#define TRC(msg)
#define TRC0(msg)
#define TRC1(msg,p1)

#endif


#ifdef ENABLE_ERROR_LOGGING

#define LOG(msg) \
  cout << (msg)

#define LOG0(msg) \
  cout << (msg) << endl

#define LOG1(msg,p1) \
  cout << (msg) << (p1) << endl

#else

#define LOG(msg)
#define LOG0(msg)
#define LOG1(msg,p1)

#endif


enum
{
  UIC_SEEK_SET,
  UIC_SEEK_CUR,
  UIC_SEEK_END
};

typedef unsigned int uic_size_t;

typedef enum _JPEG_MODE
{
  JPEG_UNKNOWN     = 0,
  JPEG_BASELINE    = 1,
  JPEG_EXTENDED    = 2,
  JPEG_PROGRESSIVE = 3,
  JPEG_LOSSLESS    = 4

} JMODE;


typedef enum _JPEG_DCT_SCALE
{
  JD_1_1  = 0,
  JD_1_2  = 1,
  JD_1_4  = 2,
  JD_1_8  = 3

} JDD;

typedef enum _JPEG_DATA_ORDER
{
  JD_PIXEL = 0,
  JD_PLANE = 1

} JDORDER;

typedef enum _JPEG_OPERATION
{
  JO_READ_HEADER = 0,
  JO_READ_DATA   = 1

} JOPERATION;

typedef enum _JPEG_THREADING_MODE
{
  JT_OLD  = 0,
  JT_RSTI = 1

} JTMODE;

typedef enum _JPEG_COLOR
{
  JC_UNKNOWN = 0,
  JC_GRAY    = 1,
  JC_RGB     = 2,
  JC_BGR     = 3,
  JC_YCBCR   = 4,
  JC_CMYK    = 5,
  JC_YCCK    = 6,
  JC_BGRA    = 7,
  JC_RGBA    = 8,

  JC_IMC3    = 9,
  JC_NV12    = 10,

  // Number of available JPEG colors
  JC_MAX

} JCOLOR;


typedef enum _JPEG_SAMPLING
{
  JS_444   = 0,
  JS_422H  = 1,
  JS_422V  = 2,
  JS_420   = 3,
  JS_411   = 4,
  JS_OTHER = 5

} JSS;


typedef enum _JPEG_MARKER
{
  JM_NONE  = 0,
  JM_SOI   = 0xd8, /* start of image */
  JM_EOI   = 0xd9, /* end of image */

  JM_TEM   = 0x01, /* For temporary private use in arithmetic coding */

  /* start of frame */
  JM_SOF0  = 0xc0, /* Nondifferential Huffman-coding Baseline DCT */
  JM_SOF1  = 0xc1, /* Nondifferential Huffman-coding Extended sequental DCT */
  JM_SOF2  = 0xc2, /* Nondifferential Huffman-coding Progressive DCT */
  JM_SOF3  = 0xc3, /* Nondifferential Huffman-coding Lossless (sequental) */

  JM_SOF5  = 0xc5, /* Differential Huffman-coding Sequental DCT */
  JM_SOF6  = 0xc6, /* Differential Huffman-coding Progressive DCT */
  JM_SOF7  = 0xc7, /* Differential Lossless */

  JM_SOF9  = 0xc9, /* Nondifferential arithmetic-coding Extended sequental DCT */
  JM_SOFA  = 0xca, /* Nondifferential arithmetic-coding Progressive DCT */
  JM_SOFB  = 0xcb, /* Nondifferential arithmetic-coding Lossless (sequental) */

  JM_SOFD  = 0xcd, /* Differential arithmetic-coding Sequental DCT */
  JM_SOFE  = 0xce, /* Differential arithmetic-coding Progressive DCT */
  JM_SOFF  = 0xcf, /* Differential arithmetic-coding Lossless */

  JM_SOS   = 0xda, /* start of scan */
  JM_DQT   = 0xdb, /* define quantization table(s) */
  JM_DHT   = 0xc4, /* define Huffman table(s) */
  JM_APP0  = 0xe0, /* APP0 */
  JM_APP1  = 0xe1,
  JM_APP2  = 0xe2,
  JM_APP3  = 0xe3,
  JM_APP4  = 0xe4,
  JM_APP5  = 0xe5,
  JM_APP6  = 0xe6,
  JM_APP7  = 0xe7,
  JM_APP8  = 0xe8,
  JM_APP9  = 0xe9,
  JM_APP10 = 0xea,
  JM_APP11 = 0xeb,
  JM_APP12 = 0xec,
  JM_APP13 = 0xed,
  JM_APP14 = 0xee, /* APP14 */
  JM_APP15 = 0xef,
  JM_RST0  = 0xd0, /* restart with modulo 8 counter 0 */
  JM_RST1  = 0xd1, /* restart with modulo 8 counter 1 */
  JM_RST2  = 0xd2, /* restart with modulo 8 counter 2 */
  JM_RST3  = 0xd3, /* restart with modulo 8 counter 3 */
  JM_RST4  = 0xd4, /* restart with modulo 8 counter 4 */
  JM_RST5  = 0xd5, /* restart with modulo 8 counter 5 */
  JM_RST6  = 0xd6, /* restart with modulo 8 counter 6 */
  JM_RST7  = 0xd7, /* restart with modulo 8 counter 7 */
  JM_DRI   = 0xdd, /* define restart interval */
  JM_COM   = 0xfe  /* comment */

} JMARKER;


typedef enum _JPEG_ERROR
{
  JPEG_OK              =   0,
  JPEG_NOT_IMPLEMENTED =  -1,
  JPEG_ERR_INTERNAL    =  -2,
  JPEG_ERR_PARAMS      =  -3,
  JPEG_ERR_BUFF        =  -4,
  JPEG_ERR_FILE        =  -5,
  JPEG_ERR_ALLOC       =  -6,
  JPEG_ERR_BAD_DATA    =  -7,
  JPEG_ERR_SOF_DATA    =  -8,
  JPEG_ERR_DQT_DATA    =  -9,
  JPEG_ERR_DHT_DATA    = -10,
  JPEG_ERR_SOS_DATA    = -11,
  JPEG_ERR_RST_DATA    = -12

} JERRCODE;


typedef enum _HTBL_CLASS
{
  DC = 0,
  AC = 1

} HTBL_CLASS;


typedef enum _JPEG_RES_UNITS
{
  JRU_NONE = 0,
  JRU_DPI  = 1,  // dot per inch
  JRU_DPC  = 2   // dot per cm

} JRESUNITS;

typedef struct _image
{
  union
  {
    uint8_t*  Data8u[4];
    int16_t* Data16s[4];
  } p;

  int     width;
  int     height;
  int     lineStep[4];
  int     precision;
  int     nChannels;
  JCOLOR  color;
  JSS     sampling;
  JDORDER order;

} IMAGE;

typedef struct _jscan
{
    int    scan_no;
    int    jpeg_restart_interval;
    int    min_h_factor;
    int    min_v_factor;
    uint32_t numxMCU;
    uint32_t numyMCU;
    int    mcuWidth;
    int    mcuHeight;
    int    xPadding;
    int    yPadding;
    int    ncomps;
    int    first_comp;

} JSCAN;

const int CPU_CACHE_LINE      = 32;
const int DCTSIZE             = 8;
const int DCTSIZE2            = 64;
const int MAX_QUANT_TABLES    = 4;
const int MAX_HUFF_TABLES     = 4;
const int MAX_COMPS_PER_SCAN  = 4;
const int MAX_COMPS_PER_FRAME = 255;
const int MAX_SCANS_PER_FRAME = 3;
const int MAX_HUFF_BITS       = 16;
const int MAX_HUFF_VALS       = 256;
const int MAX_BLOCKS_PER_MCU  = 10;
const int MAX_BYTES_PER_MCU   = DCTSIZE2 * sizeof(int16_t) * MAX_BLOCKS_PER_MCU;
const int SAFE_NBYTES         = 128;


extern const uint8_t DefaultLuminanceQuant[64];
extern const uint8_t DefaultChrominanceQuant[64];

extern const uint8_t DefaultLuminanceDCBits[];
extern const uint8_t DefaultLuminanceDCValues[];
extern const uint8_t DefaultChrominanceDCBits[];
extern const uint8_t DefaultChrominanceDCValues[];
extern const uint8_t DefaultLuminanceACBits[];
extern const uint8_t DefaultLuminanceACValues[];
extern const uint8_t DefaultChrominanceACBits[];
extern const uint8_t DefaultChrominanceACValues[];

const vm_char* GetErrorStr(JERRCODE code);

int  get_num_threads(void);
void set_num_threads(int maxThreads);


class CMemoryBuffer
{
public:
  CMemoryBuffer(void) { m_buffer_size = 0; m_buffer = 0; }
  virtual ~CMemoryBuffer(void) { Delete(); }

  JERRCODE Allocate(int size);
  JERRCODE Delete(void);

  operator uint8_t*() { return m_buffer; }

  int     m_buffer_size;
  uint8_t*  m_buffer;

};

enum ChromaType
{
    CHROMA_TYPE_YUV400         = 0, // (grayscale image)
    CHROMA_TYPE_YUV420         = 1, // Y: h=2 v=2, Cb/Cr: h=1 v=1
    CHROMA_TYPE_YUV422H_2Y     = 2, // Y: h=2 v=1, Cb/Cr: h=1 v=1
    CHROMA_TYPE_YUV444         = 3, // Y: h=1 v=1, Cb/Cr: h=1 v=1
    CHROMA_TYPE_YUV411         = 4, // Y: h=4 v=1, Cb/Cr: h=1 v=1
    CHROMA_TYPE_YUV422V_2Y     = 5, // Y: h=1 v=2, Cb/Cr: h=1 v=1
    CHROMA_TYPE_YUV422H_4Y     = 6, // Y: h=2 v=1, Cb/Cr: h=1 v=2
    CHROMA_TYPE_YUV422V_4Y     = 7, // Y: h=1 v=2, Cb/Cr: h=2 v=1
    CHROMA_TYPE_RGB            = 8, // Y: h=1 v=1, Cb/Cr: h=1 v=1
    CHROMA_TYPE_BGR            = 9  // Y: h=1 v=1, Cb/Cr: h=1 v=1
};

#endif // MFX_ENABLE_MJPEG_VIDEO_DECODE || MFX_ENABLE_MJPEG_VIDEO_ENCODE
#endif // __JPEGBASE_H__


