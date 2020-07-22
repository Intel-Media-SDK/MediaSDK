// Copyright (c) 2017-2020 Intel Corporation
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

#ifdef MFX_ENABLE_AV1_VIDEO_DECODE

#ifndef __UMC_AV1_BITSTREAM_UTILS_H_
#define __UMC_AV1_BITSTREAM_UTILS_H_

#include <limits>
#include "mfx_utils.h"
#include "umc_av1_bitstream.h"
#include "umc_vp9_utils.h"

namespace UMC_AV1_DECODER
{
    inline int read_inv_signed_literal(AV1Bitstream& bs, int bits)
    {
        const unsigned sign = bs.GetBit();
        const unsigned literal = bs.GetBits(bits);
        if (sign == 0)
            return literal; // if positive: literal
        else
            return literal - (1 << bits); // if negative: complement to literal with respect to 2^bits
    }

    inline int32_t read_uniform(AV1Bitstream& bs, uint32_t n)
    {
        const uint32_t l = UMC_VP9_DECODER::GetUnsignedBits(n);
        const uint32_t m = (1 << l) - n;
        const uint32_t v = bs.GetBits(l - 1);
        if (v < m)
            return v;
        else
            return (v << 1) - m + bs.GetBits(1);
    }

    inline uint32_t read_uvlc(AV1Bitstream& bs)
    {
        uint32_t leading_zeros = 0;
        while (!bs.GetBit()) ++leading_zeros;

        // Maximum 32 bits.
        if (leading_zeros >= 32)
            return std::numeric_limits<uint32_t>::max();
        const uint32_t base = (1u << leading_zeros) - 1;
        const uint32_t value = bs.GetBits(leading_zeros);
        return base + value;
    }

    inline uint16_t inv_recenter_non_neg(uint16_t r, uint16_t v)
    {
        if (v >(r << 1))
            return v;
        else if ((v & 1) == 0)
            return (v >> 1) + r;
        else
            return r - ((v + 1) >> 1);
    }

    inline uint16_t inv_recenter_finite_non_neg(uint16_t n, uint16_t r, uint16_t v) {
        if ((r << 1) <= n) {
            return inv_recenter_non_neg(r, v);
        }
        else {
            return n - 1 - inv_recenter_non_neg(n - 1 - r, v);
        }
    }

    inline uint8_t get_msb(uint32_t n)
    {
        if (n == 0)
            throw av1_exception(UMC::UMC_ERR_FAILED);

        uint8_t pos = 0;
        while (n > 1)
        {
            pos++;
            n >>= 1;
        }

        return pos;
    }

    inline uint16_t read_primitive_quniform(AV1Bitstream& bs, uint16_t n)
    {
        if (n <= 1) return 0;
        uint8_t l = get_msb(n - 1) + 1;
        const int m = (1 << l) - n;
        const int v = bs.GetBits(l - 1);
        const int result = v < m ? v : (v << 1) - m + bs.GetBit();
        return static_cast<uint16_t>(result);
    }

    inline uint16_t read_primitive_subexpfin(AV1Bitstream& bs, uint16_t n, uint16_t k)
    {
        int i = 0;
        int mk = 0;
        uint16_t v;
        while (1) {
            int b = (i ? k + i - 1 : k);
            int a = (1 << b);
            if (n <= mk + 3 * a) {
                v = static_cast<uint16_t>(read_primitive_quniform(bs, static_cast<uint16_t>(n - mk)) + mk);
                break;
            }
            else {
                if (bs.GetBit()) {
                    i = i + 1;
                    mk += a;
                }
                else {
                    v = static_cast<uint16_t>(bs.GetBits(b) + mk);
                    break;
                }
            }
        }
        return v;
    }

    inline uint16_t read_primitive_refsubexpfin(AV1Bitstream& bs, uint16_t n, uint16_t k, uint16_t ref)
    {
        return inv_recenter_finite_non_neg(n, ref,
            read_primitive_subexpfin(bs, n, k));
    }

    inline int16_t read_signed_primitive_refsubexpfin(AV1Bitstream& bs, uint16_t n, uint16_t k, int16_t ref) {
        ref += n - 1;
        const uint16_t scaled_n = (n << 1) - 1;
        return read_primitive_refsubexpfin(bs, scaled_n, k, ref) - n + 1;
    }

    inline size_t read_leb128(AV1Bitstream& bs)
    {
        size_t value = 0;
        for (size_t i = 0; i < MAX_LEB128_SIZE; ++i)
        {
            const uint8_t cur_byte = static_cast<uint8_t>(bs.GetBits(8));
            const uint8_t decoded_byte = cur_byte & LEB128_BYTE_MASK;
            value |= ((uint64_t)decoded_byte) << (i * 7);
            if ((cur_byte & ~LEB128_BYTE_MASK) == 0)
                return value;
        }

        throw av1_exception(UMC::UMC_ERR_INVALID_STREAM);
    }

    const uint8_t DIV_LUT_PREC_BITS = 14;
    const uint8_t DIV_LUT_BITS = 8;
    const uint16_t DIV_LUT_NUM = (1 << DIV_LUT_BITS);

    static const uint16_t div_lut[DIV_LUT_NUM + 1] = {
        16384, 16320, 16257, 16194, 16132, 16070, 16009, 15948, 15888, 15828, 15768,
        15709, 15650, 15592, 15534, 15477, 15420, 15364, 15308, 15252, 15197, 15142,
        15087, 15033, 14980, 14926, 14873, 14821, 14769, 14717, 14665, 14614, 14564,
        14513, 14463, 14413, 14364, 14315, 14266, 14218, 14170, 14122, 14075, 14028,
        13981, 13935, 13888, 13843, 13797, 13752, 13707, 13662, 13618, 13574, 13530,
        13487, 13443, 13400, 13358, 13315, 13273, 13231, 13190, 13148, 13107, 13066,
        13026, 12985, 12945, 12906, 12866, 12827, 12788, 12749, 12710, 12672, 12633,
        12596, 12558, 12520, 12483, 12446, 12409, 12373, 12336, 12300, 12264, 12228,
        12193, 12157, 12122, 12087, 12053, 12018, 11984, 11950, 11916, 11882, 11848,
        11815, 11782, 11749, 11716, 11683, 11651, 11619, 11586, 11555, 11523, 11491,
        11460, 11429, 11398, 11367, 11336, 11305, 11275, 11245, 11215, 11185, 11155,
        11125, 11096, 11067, 11038, 11009, 10980, 10951, 10923, 10894, 10866, 10838,
        10810, 10782, 10755, 10727, 10700, 10673, 10645, 10618, 10592, 10565, 10538,
        10512, 10486, 10460, 10434, 10408, 10382, 10356, 10331, 10305, 10280, 10255,
        10230, 10205, 10180, 10156, 10131, 10107, 10082, 10058, 10034, 10010, 9986,
        9963,  9939,  9916,  9892,  9869,  9846,  9823,  9800,  9777,  9754,  9732,
        9709,  9687,  9664,  9642,  9620,  9598,  9576,  9554,  9533,  9511,  9489,
        9468,  9447,  9425,  9404,  9383,  9362,  9341,  9321,  9300,  9279,  9259,
        9239,  9218,  9198,  9178,  9158,  9138,  9118,  9098,  9079,  9059,  9039,
        9020,  9001,  8981,  8962,  8943,  8924,  8905,  8886,  8867,  8849,  8830,
        8812,  8793,  8775,  8756,  8738,  8720,  8702,  8684,  8666,  8648,  8630,
        8613,  8595,  8577,  8560,  8542,  8525,  8508,  8490,  8473,  8456,  8439,
        8422,  8405,  8389,  8372,  8355,  8339,  8322,  8306,  8289,  8273,  8257,
        8240,  8224,  8208,  8192,
    };

    /* Shift down with rounding for use when n >= 0, value >= 0 */
    inline uint64_t round_power_of_two(uint64_t value, uint16_t n)
    {
        return (value + ((static_cast<uint64_t>(1) << n) >> 1)) >> n;
    }

    /* Shift down with rounding for signed integers, for use when n >= 0 */
    inline int64_t round_power_of_two_signed(int64_t value, uint16_t n)
    {
        return (value < 0) ? -(static_cast<int64_t>(round_power_of_two(-value, n)))
            : round_power_of_two(value, n);
    }

    // Decomposes a divisor D such that 1/D = y/2^shift, where y is returned
    // at precision of DIV_LUT_PREC_BITS along with the shift.
    inline int16_t resolve_divisor_32(uint32_t D, uint16_t& shift)
    {
        int32_t f;
        shift = get_msb(D);
        // e is obtained from D after resetting the most significant 1 bit.
        const int32_t e = D - (static_cast < uint32_t>(1) << shift);
        // Get the most significant DIV_LUT_BITS (8) bits of e into f
        if (shift > DIV_LUT_BITS)
            f = static_cast<uint32_t>(round_power_of_two(e, shift - DIV_LUT_BITS));
        else
            f = e << (DIV_LUT_BITS - shift);
        VM_ASSERT(f <= DIV_LUT_NUM);
        shift += DIV_LUT_PREC_BITS;
        // Use f as lookup into the precomputed table of multipliers
        return div_lut[f];
    }

    inline bool is_affine_valid(GlobalMotionParams const & params)
    {
        return (params.wmmat[2] > 0);
    }

    inline bool is_affine_shear_allowed(int16_t alpha, int16_t beta, int16_t gamma, int16_t delta)
    {
        if ((4 * abs(alpha) + 7 * abs(beta) >= (1 << WARPEDMODEL_PREC_BITS)) ||
            (4 * abs(gamma) + 4 * abs(delta) >= (1 << WARPEDMODEL_PREC_BITS)))
            return false;
        else
            return true;
    }

    inline bool av1_get_shear_params(GlobalMotionParams& params)
    {
        if (!is_affine_valid(params))
            return false;

        const int32_t int16Min = std::numeric_limits<int16_t>::min();
        const int32_t int16Max = std::numeric_limits<int16_t>::max();

        params.alpha =
            static_cast<int16_t>(mfx::clamp(params.wmmat[2] - (1 << WARPEDMODEL_PREC_BITS), int16Min, int16Max));
        params.beta = static_cast<int16_t>(mfx::clamp(params.wmmat[3], int16Min, int16Max));
        uint16_t shift;
        int16_t y = resolve_divisor_32(abs(params.wmmat[2]), shift) * (params.wmmat[2] < 0 ? -1 : 1);
        int64_t v = ((int64_t)params.wmmat[4] * (1 << WARPEDMODEL_PREC_BITS)) * y;
        params.gamma =
            static_cast<int16_t>(mfx::clamp((int)round_power_of_two_signed(v, shift), int16Min, int16Max));
        v = ((int64_t)params.wmmat[3] * params.wmmat[4]) * y;
        params.delta = static_cast<int16_t>(mfx::clamp(params.wmmat[5] - (int)round_power_of_two_signed(v, shift) -
            (1 << WARPEDMODEL_PREC_BITS),
            int16Min, int16Max));

        params.alpha = static_cast<int16_t>(round_power_of_two_signed(params.alpha, WARP_PARAM_REDUCE_BITS) *
            (1 << WARP_PARAM_REDUCE_BITS));
        params.beta = static_cast<int16_t>(round_power_of_two_signed(params.beta, WARP_PARAM_REDUCE_BITS) *
            (1 << WARP_PARAM_REDUCE_BITS));
        params.gamma = static_cast<int16_t>(round_power_of_two_signed(params.gamma, WARP_PARAM_REDUCE_BITS) *
            (1 << WARP_PARAM_REDUCE_BITS));
        params.delta = static_cast<int16_t>(round_power_of_two_signed(params.delta, WARP_PARAM_REDUCE_BITS) *
            (1 << WARP_PARAM_REDUCE_BITS));

        if (!is_affine_shear_allowed(params.alpha, params.beta, params.gamma, params.delta))
            return false;

        return true;
    }
}

#endif // __UMC_AV1_BITSTREAM_H_

#endif // MFX_ENABLE_AV1_VIDEO_DECODE
