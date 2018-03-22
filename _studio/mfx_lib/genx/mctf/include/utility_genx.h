/*//////////////////////////////////////////////////////////////////////////////
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
*/

#ifndef CMUT_IO_GENX_H
#define CMUT_IO_GENX_H
namespace cmut {
    template <uint R, uint C, typename T>
    inline _GENX_ bool cmut_write(SurfaceIndex &s, uint x, uint y, matrix<T, R, C> &m) {
        return cmut_write(s, x, y, m.select_all());
    }

    template <uint R, uint C, typename T>
    inline _GENX_ void cmut_read(SurfaceIndex &s, uint x, uint y, matrix<T, R, C> &m) {
        cmut_read(s, x, y, m.select_all());
    }

    template <uint R, uint C, typename T>
    inline _GENX_ typename std::enable_if<C*sizeof(T) <= 32 && R*C*sizeof(T) <= 256, void>::type cmut_read(SurfaceIndex &s, uint x, uint y, matrix_ref<T, R, C> m) {
        read(s, x, y, m);
    }

    template <uint R, uint C, typename T>
    inline _GENX_ typename std::enable_if<(C*sizeof(T) <= 32) && (R*C*sizeof(T)>256), void > ::type cmut_read(SurfaceIndex &s, uint x, uint y, matrix_ref<T, R, C> m) {
        cmut_read(s, x, y, m.template select<    256 / (C*sizeof(T)), 1, C, 1>(0, 0));
        cmut_read(s, x, y + (256 / (C*sizeof(T))), m.template select<R - 256 / (C*sizeof(T)), 1, C, 1>(256 / (C*sizeof(T)), 0));
    }

    template <uint R, uint C, typename T>
    inline _GENX_ typename std::enable_if<(C*sizeof(T)>32), void > ::type cmut_read(SurfaceIndex &s, uint x, uint y, matrix_ref<T, R, C> m) {
        cmut_read(s, x, y, m.template select<R, 1, 32 / sizeof(T), 1>(0, 0));
        cmut_read(s, x + 32, y, m.template select<R, 1, C - 32 / sizeof(T), 1>(0, 32 / sizeof(T)));
    }

    template <uint R, uint C, typename T>
    inline _GENX_ void cmut_read_plane(SurfaceIndex &s, CmSurfacePlaneIndex PID, uint x, uint y, matrix<T, R, C> &m) {
        cmut_read_plane(s, PID, x, y, m.select_all());
    }

    template <uint R, uint C, typename T>
    inline _GENX_ typename std::enable_if<C*sizeof(T) <= 32 && R*C*sizeof(T) <= 256, void>::type cmut_read_plane(SurfaceIndex &s, CmSurfacePlaneIndex PID, uint x, uint y, matrix_ref<T, R, C> m) {
        read_plane(s, PID, x, y, m);
    }

    template <uint R, uint C, typename T>
    inline _GENX_ typename std::enable_if<(C*sizeof(T) <= 32) && (R*C*sizeof(T)>256), void > ::type cmut_read_plane(SurfaceIndex &s, CmSurfacePlaneIndex PID, uint x, uint y, matrix_ref<T, R, C> m) {
        cmut_read_plane(s, PID, x, y, m.template select<    256 / (C*sizeof(T)), 1, C, 1>(0, 0));
        cmut_read_plane(s, PID, x, y + (256 / (C*sizeof(T))), m.template select<R - 256 / (C*sizeof(T)), 1, C, 1>(256 / (C*sizeof(T)), 0));
    }

    template <uint R, uint C, typename T>
    inline _GENX_ typename std::enable_if<(C*sizeof(T)>32), void > ::type cmut_read_plane(SurfaceIndex &s, CmSurfacePlaneIndex PID, uint x, uint y, matrix_ref<T, R, C> m) {
        cmut_read_plane(s, PID, x, y, m.template select<R, 1, 32 / sizeof(T), 1>(0, 0));
        cmut_read_plane(s, PID, x + 32, y, m.template select<R, 1, C - 32 / sizeof(T), 1>(0, 32 / sizeof(T)));
    }

    template <uint R, uint C, typename T>
    inline _GENX_ typename std::enable_if<C*sizeof(T) <= 32 && R*C*sizeof(T) <= 256, bool>::type cmut_write(SurfaceIndex &s, uint x, uint y, matrix_ref<T, R, C> m) {
        return write(s, x, y, m);
    }

    template <uint R, uint C, typename T>
    inline _GENX_ typename std::enable_if<(C*sizeof(T) <= 32 && R*C*sizeof(T)>256 && R % ((256 / (C*sizeof(T)))) == 0), bool > ::type cmut_write(SurfaceIndex &s, uint x, uint y, matrix_ref<T, R, C> m) {
        bool r = true;
#pragma unroll
        for (int i = 0; i<R; i += (256 / (C*sizeof(T)))) {
            r = r & write(s, x, y + i, m.template select<(256 / (C*sizeof(T))), 1, C, 1>(i, 0));
        }
        return r;
    }

    template <uint R, uint C, typename T>
    inline _GENX_ typename std::enable_if<(C*sizeof(T) <= 32 && R*C*sizeof(T)>256 && R % (256 / (C*sizeof(T))) != 0), bool>::type cmut_write(SurfaceIndex &s, uint x, uint y, matrix_ref<T, R, C> m) {
        int i = 0;
        bool r = true;
#pragma unroll
        for (; i<R - R % (256 / (C*sizeof(T))); i += (256 / (C*sizeof(T)))) {
            static_assert(256 / (C*sizeof(T))*C <= 256, "Wrong");
            r = r & write(s, x, y + i, m.template select<(256 / (C*sizeof(T))), 1, C, 1>(i, 0));
        }
        r = r & write(s, x, y + i, m.template select<R % (256 / (C*sizeof(T))), 1, C, 1>(i, 0));
        return r;
    }

    template <uint R, uint C, typename T>
    inline _GENX_ typename std::enable_if<(C*sizeof(T)>32), bool>::type cmut_write(SurfaceIndex &s, uint x, uint y, matrix_ref<T, R, C> m) {
        bool r = cmut_write(s, x, y, m.template select<R, 1, 32 / sizeof(T), 1>(0, 0));
        r = r & cmut_write(s, x + 32, y, m.template select<R, 1, C - 32 / sizeof(T), 1>(0, 32 / sizeof(T)));
        return r;
    }

    template <uint R, uint C, typename T>
    inline _GENX_ typename std::enable_if<C*sizeof(T) <= 32 && R*C*sizeof(T) <= 256, bool>::type cmut_write_plane(SurfaceIndex &s, CmSurfacePlaneIndex PID, uint x, uint y, matrix_ref<T, R, C> m) {
        return write_plane(s, PID, x, y, m);
    }

    template <uint R, uint C, typename T>
    inline _GENX_ typename std::enable_if<(C*sizeof(T) <= 32 && R*C*sizeof(T)>256 && R % ((256 / (C*sizeof(T)))) == 0), bool > ::type cmut_write_plane(SurfaceIndex &s, CmSurfacePlaneIndex PID, uint x, uint y, matrix_ref<T, R, C> m) {
        bool r = true;
#pragma unroll
        for (int i = 0; i<R; i += (256 / (C*sizeof(T)))) {
            r = r & write_plane(s, PID, x, y + i, m.template select<(256 / (C*sizeof(T))), 1, C, 1>(i, 0));
        }
        return r;
    }

    template <uint R, uint C, typename T>
    inline _GENX_ typename std::enable_if<(C*sizeof(T) <= 32 && R*C*sizeof(T)>256 && R % (256 / (C*sizeof(T))) != 0), bool>::type cmut_write_plane(SurfaceIndex &s, CmSurfacePlaneIndex PID, uint x, uint y, matrix_ref<T, R, C> m) {
        int i = 0;
        bool r = true;
#pragma unroll
        for (; i<R - R % (256 / (C*sizeof(T))); i += (256 / (C*sizeof(T)))) {
            static_assert(256 / (C*sizeof(T))*C <= 256, "Wrong");
            r = r & write_plane(s, PID, x, y + i, m.template select<(256 / (C*sizeof(T))), 1, C, 1>(i, 0));
        }
        r = r & write_plane(s, x, PID, y + i, m.template select<R % (256 / (C*sizeof(T))), 1, C, 1>(i, 0));
        return r;
    }

    template <uint R, uint C, typename T>
    inline _GENX_ typename std::enable_if<(C*sizeof(T)>32), bool>::type cmut_write_plane(SurfaceIndex &s, CmSurfacePlaneIndex PID, uint x, uint y, matrix_ref<T, R, C> m) {
        bool r = cmut_write_plane(s, PID, x, y, m.template select<R, 1, 32 / sizeof(T), 1>(0, 0));
        r = r & cmut_write_plane(s, PID, x + 32, y, m.template select<R, 1, C - 32 / sizeof(T), 1>(0, 32 / sizeof(T)));
        return r;
    }

    template <uint SZ, typename Tdata>
    inline _GENX_ typename std::enable_if<(SZ>16), void > ::type cmut_slm_atomic(uint slmid, CmAtomicOpType aop, vector<ushort, SZ> &v_Addr, vector<Tdata, SZ> &v_Dst, vector<Tdata, SZ> &v_Src0) {
        {
            vector<ushort, 16> v_addr0 = v_Addr.template select<16, 1>(0);
            vector<Tdata, 16> v_Dst0 = v_Dst.template select<16, 1>(0);
            vector<Tdata, 16> v_Src00 = v_Src00.template select<16, 1>(0);
            cmut_slm_atomic(slmid, aop, v_addr0, v_Dst0, v_Src00);
        }
        {
            vector<ushort, SZ - 16> v_addr0 = v_Addr.template select<SZ - 16, 1>(0);
            vector<Tdata, SZ - 16> v_Dst0 = v_Dst.template select<SZ - 16, 1>(0);
            vector<Tdata, SZ - 16> v_Src00 = v_Src00.template select<SZ - 16, 1>(0);
            cmut_slm_atomic(slmid, aop, v_addr0, v_Dst0, v_Src00);
        }
    }

    template <uint SZ, typename Tdata>
    inline _GENX_ typename std::enable_if<(SZ <= 16), void>::type cmut_slm_atomic(uint slmid, CmAtomicOpType aop, vector<ushort, SZ> &v_Addr, vector<Tdata, SZ> &v_Dst, vector<Tdata, SZ> &v_Src0) {
        cm_slm_atomic(slmid, aop, v_Addr, v_Dst, v_Src0);
    }

    template <uint RR, typename T, uint R, uint C>
    inline _GENX_ matrix_ref<T, RR, C> cmut_rows(matrix<T, R, C> &m, uint r0) {
        return cmut_rows<RR>(m.select_all(), r0);
    }

    template <uint RR, typename T, uint R, uint C>
    inline _GENX_ matrix_ref<T, RR, C> cmut_rows(matrix_ref<T, R, C> m, uint r0) {
        return m.template select<RR, 1, C, 1>(r0, 0);
    }

    template <uint RC, typename T, uint R, uint C>
    inline _GENX_ matrix_ref<T, R, RC> cmut_cols(matrix<T, R, C> &m, uint c0) {
        return cmut_cols<RC>(m.select_all(), c0);
    }

    template <uint RC, typename T, uint R, uint C>
    inline _GENX_ matrix_ref<T, R, RC> cmut_cols(matrix_ref<T, R, C> m, uint c0) {
        return m.template select<R, 1, RC, 1>(0, c0);
    }

    template <uint RSZ, typename T, uint SZ>
    inline _GENX_ vector_ref<T, RSZ> cmut_slice(vector<T, SZ> &v, uint i0) {
        return cmut_slice<RSZ>(v.template select<SZ, 1U>() , i0);
    }

    template <uint RSZ, typename T, uint SZ>
    inline _GENX_ vector_ref<T, RSZ> cmut_slice(vector_ref<T, SZ> v, uint i0) {
        return v.template select<RSZ, 1>(i0);
    }

}
#endif