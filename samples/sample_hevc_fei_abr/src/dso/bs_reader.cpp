// Copyright (c) 2018 Intel Corporation
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

#include <bs_reader.h>

#define BS_SKIP_IGNORED_BYTES() \
    if( ignore_bytes_cnt && next_ignored_byte ) \
        if( *next_ignored_byte == (file_pos+offset) ){ \
            offset++; \
            next_ignored_byte++; \
            ignore_bytes_cnt--; \
            if( buf_size < offset+readBytes+((lastBits)?1:0) ){ \
                if(read_more_data()) return last_err; \
        } \
    }

Bs32u BS_Reader::ue_v(){
    Bs32s leadingZeroBits = -1;
    for( byte b = 0; !b; leadingZeroBits++ ) b = read_1_bit();
    return (1<<leadingZeroBits) + read_bits( leadingZeroBits ) - 1;
}

Bs32s BS_Reader::se_v(){
    Bs32u val = ue_v();
    return (val&0x01) ? (Bs32s)((val+1)>>1) : -(Bs32s)((val+1)>>1);
}

BSErr BS_Reader::read_more_data(){
    if( !bs ) return last_err = BS_ERR_MORE_DATA;
    if( BS_FEOF(bs) ) return last_err = BS_ERR_MORE_DATA;
    Bs32u keep_bytes = buf_size-offset;
    if( keep_bytes == buf_size ) return last_err = BS_ERR_NOT_ENOUGH_BUFFER;
    if( offset < buf_size ) memcpy( buf, buf+offset, keep_bytes );
    file_pos = BS_FTELL(bs) - keep_bytes;
    if( !(buf_size = (Bs32u)BS_FREAD( bs, buf+keep_bytes, offset)) ) /*return*/ last_err = BS_ERR_MORE_DATA;
    buf_size += keep_bytes;
    offset = 0;
    return last_err;
}

byte BS_Reader::read_1_bit(){
    if(!bit){
        // skip start code emulation prevention byte
        if( ignore_bytes_cnt && next_ignored_byte ){
            if( *next_ignored_byte == (file_pos+offset) ){
                if (buf_size == offset)
                    if (read_more_data())
                        return last_err;
                offset++;
                next_ignored_byte++;
                ignore_bytes_cnt--;
            }
        }
        if( buf_size <= offset ){
            if(read_more_data()) return last_err;
        }
    }
    byte res = ((buf[offset] >> (7 - bit)) & 1);
    bit = (bit+1)&0x07;
    offset += !bit;

    return res;
}

byte BS_Reader::read_1_byte(){
    if( buf_size == offset ){
        if(read_more_data()) return (byte)last_err;
    };
    if( !bit ) return buf[offset++];
    offset++;
    return (buf[offset-1]<<bit)|(buf[offset]>>(8-bit));
}

Bs32u BS_Reader::next_bits( Bs32u nBits /*1..32*/ ){
    byte   haveBits   = bit ? (8-bit) : 0; // already in buffer
    byte   lockedByte = bit ? 1 : 0;
    Bs32u readBytes  = (byte)(nBits - haveBits + 7)>>3; //bytes to read
    byte   lastBits   = (byte)( readBytes ? ((nBits - haveBits)%8) : nBits ); //bits to read in last byte
    Bs32u tmp_offset = offset;
    Bs32u res = 0;

    if( readBytes ){
        if( buf_size < offset+lockedByte+readBytes ){
            if(read_more_data()) return last_err;
            tmp_offset = offset;
        };

        if( lockedByte ){
            res = (byte)( buf[tmp_offset]<<bit )>>bit;
            tmp_offset++;
        }

        if( lastBits )readBytes--;
        while( readBytes-- ){
            res = (res<<8)|(byte)(buf[tmp_offset]);
            tmp_offset++;
        }
        if( lastBits ){
            res = (res<<lastBits)|(byte)(buf[tmp_offset]>>(8-lastBits));
        }
    }else if( lastBits ){
        res = (byte)( buf[tmp_offset]<<bit )>>(8-lastBits);
    }
    return res;
}

Bs32u BS_Reader::next_bytes(byte nBytes /*1..4*/){
    if(buf_size < offset + nBytes){
        if(read_more_data()) return last_err;
    }

    //in order from most often invoked
    if(nBytes == 3) return (buf[offset]<<16)|(buf[offset+1]<<8)|buf[offset+2];
    else if(nBytes == 4) return (buf[offset]<<24)|(buf[offset+1]<<16)|(buf[offset+2]<<8)|buf[offset+3];
    else if(nBytes == 1) return buf[offset];
    else if(nBytes == 2) return (buf[offset]<<8)|buf[offset+1];

    return last_err = BS_ERR_INVALID_PARAMS;
}


Bs32u BS_Reader::read_bits( Bs32u nBits /*1..32*/){
    byte  haveBits   = bit ? (8-bit) : 0; // already in buffer
    byte  lockedByte = bit ? 1 : 0;
    Bs32u readBytes  = (byte)(nBits - haveBits + 7)>>3; //bytes to read
    byte  lastBits   = (byte)( readBytes ? ((nBits - haveBits)%8) : nBits ); //bits to read in last byte
    Bs32u res = 0;

    if( readBytes ){
        if( buf_size < offset+lockedByte+readBytes ){
            if(read_more_data()) return last_err;
        }

        if( lockedByte ){
            res = (byte)( buf[offset]<<bit )>>bit;
            offset++;
        }

        if( lastBits )readBytes--;
        while( readBytes-- ){
            BS_SKIP_IGNORED_BYTES();
            res = (res<<8)|(byte)(buf[offset]);
            offset++;
        }
        BS_SKIP_IGNORED_BYTES();

        if( lastBits ){
            res = (res<<lastBits)|(byte)(buf[offset]>>(8-lastBits));
        }

        bit = lastBits;

    }else if( lastBits ){
        if( buf_size <= offset ){
            if(read_more_data()) return last_err;
        }
        res = (byte)( buf[offset]<<bit )>>(8-lastBits);
        if( (lastBits + bit)>>3 ){
            bit = 0;
            offset++;
        }else bit = (byte)(lastBits + bit );
     }

    return res;
}

BSErr BS_Reader::shift( long nBytes ){
    if(nBytes > 0)
        return shift_forward(nBytes);
    else
        return shift_back(-nBytes);

    return BS_ERR_UNKNOWN;
}

BSErr BS_Reader::shift_forward( Bs32u nBytes ){
    bit = 0;
    if( nBytes+offset > buf_size){
        if(bs){
            nBytes -= (buf_size-offset);
            BS_FSEEK( bs, nBytes, SEEK_CUR );
            buf_size = BS_BUF_SIZE; // set max buf size
            offset = buf_size;
            last_err = read_more_data();
            file_pos = BS_FTELL(bs) - buf_size;
        }else{
            return last_err = BS_ERR_MORE_DATA;
        }
    }else{
        offset += nBytes;
    }
    return BS_ERR_NONE;
}

BSErr BS_Reader::shift_back( Bs32u nBytes ){
    bit = 0;
    if( nBytes > offset ){
        if(bs){
            nBytes += (buf_size-offset);
            BS_FSEEK( bs, -(BS_FOFFSET_TYPE)nBytes, SEEK_CUR );
            buf_size = BS_BUF_SIZE; // set max buf size
            offset = buf_size;
            last_err = read_more_data();
            file_pos = BS_FTELL(bs) - buf_size;
        }else{
            return last_err = BS_ERR_MORE_DATA;
        }
    }else{
        offset -= nBytes;
    }
    return BS_ERR_NONE;
}

void BS_Reader::ignore_next_byte(){
    ignore_bytes = (BS_FADDR_TYPE*)realloc(ignore_bytes,(ignore_bytes_cnt+1)*sizeof(BS_FADDR_TYPE));
    if (!ignore_bytes)
        throw std::bad_alloc();
    ignore_bytes[ignore_bytes_cnt] = file_pos+offset;
    ignore_bytes_cnt++;
}

BSErr BS_Reader::open( const char* file_name ) {
    buf_size  = BS_BUF_SIZE;
    offset    = buf_size;
    buf = (byte*)malloc( buf_size );

    if(bs) {
        if(close()) return last_err;
    }

    BS_FOPEN(file_name, bs);

    return last_err = (bs) ? BS_ERR_NONE : BS_ERR_UNKNOWN;
}

void BS_Reader::set_buffer(byte *buf, Bs32u buf_size){
    if(bs){
        close();
        bs = NULL;
    }
    last_err  = BS_ERR_NONE;
    file_pos  = 0;
    offset    = 0;
    bit       = 0;
    ignore_bytes_cnt  = 0;
    next_ignored_byte = NULL;
    this->buf = buf;
    this->buf_size = buf_size;
}

void BS_Reader::skip_bytes( Bs32u nBytes ){
    while( Bs32u(next_ignored_byte - ignore_bytes) < ignore_bytes_cnt ){
        if( (*next_ignored_byte) > (file_pos+offset+nBytes) ) break;
        nBytes++;
        next_ignored_byte++;
    }
    shift_forward(nBytes);
}

BSErr BS_Reader::read_arr(byte *arr, Bs32u size){
    if(bit) return BS_ERR_UNKNOWN;
    while(size){
        Bs32u to_copy = BS_MIN(size, buf_size - offset);
        memcpy(arr, buf+offset, to_copy);
        size -= to_copy;
        offset += to_copy;
        if(size && read_more_data()) return last_err;
        arr += to_copy;
    }
    return last_err;
}
