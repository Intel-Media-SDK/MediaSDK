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

#pragma warning(disable: 4127)
#pragma warning(disable: 4244)
#pragma warning(disable: 4018)
#pragma warning(disable: 4189)
#pragma warning(disable: 4505)
#include <cm/cm.h>
//#include <cm/cmtl.h>
#include <cm/genx_vme.h>
#include "genx_global.h"

#define LMSAD                   500
#define WEIGHT_MULTIPLIER       8
#define SELECTION_THRESHOLD     8388608
#define MERGE_LIMIT             256
#define LONG_MV                 64
#define DISTANCETH              8

typedef unsigned int uint;
template <uint BLOCKW, uint BLOCKH>
inline _GENX_
void read_plane_ext(SurfaceIndex SURF_IN, int x, int y, matrix<uchar, BLOCKH, BLOCKW>& m_in)
{
    enum
    {
        MAX_SIZE = 256
    };

    const int count = (BLOCKH*BLOCKW + MAX_SIZE - 1)/MAX_SIZE;
    const int lines  = MAX_SIZE/BLOCKW < BLOCKH ? MAX_SIZE/BLOCKW : BLOCKH; 
    const uint done = lines*(count-1);
    const uint rest = BLOCKH - done;

    #pragma unroll
    for (int i=0; i<count-1; i++)
    {
        read_plane(SURF_IN, GENX_SURFACE_Y_PLANE, x, y+lines*i, m_in.template select<lines, 1, BLOCKW, 1>(lines*i, 0));
    }
    read_plane(SURF_IN, GENX_SURFACE_Y_PLANE, x, y+done, m_in.template select<rest, 1, BLOCKW, 1>(done, 0));
}


void _GENX_ inline VHpelInterp(SurfaceIndex SURF_REF,
    short xref, short yref, matrix<uchar, 8, 8> &m_out) {

    matrix<uchar, 11, 8> m_ref_vert;
    matrix<short, 8, 8> m_ref_bkp;
    read_plane_ext<8, 11>(SURF_REF, xref, yref - 1, m_ref_vert);
    m_ref_bkp  = (m_ref_vert.select<8,1,8,1>(1,0) + m_ref_vert.select<8,1,8,1>(2,0));
    m_ref_bkp *= 5;
    m_ref_bkp -= m_ref_vert.select<8,1,8,1>(0,0);
    m_ref_bkp -= m_ref_vert.select<8,1,8,1>(3,0);
    m_ref_bkp += 4;
    m_out = cm_shr<uchar>(m_ref_bkp,3,SAT);
}

void _GENX_ inline HHpelInterp(SurfaceIndex SURF_REF,
    short xref, short yref, matrix<uchar, 8, 8> &m_out) {

    matrix<uchar, 8, 16> m_ref_horz;
    matrix<short, 8, 8> m_ref_bkp;
    read_plane_ext<16, 8>(SURF_REF, xref - 1, yref, m_ref_horz);
    m_ref_bkp  = (m_ref_horz.select<8,1,8,1>(0,1) + m_ref_horz.select<8,1,8,1>(0,2));
    m_ref_bkp *= 5;
    m_ref_bkp -= m_ref_horz.select<8,1,8,1>(0,0);
    m_ref_bkp -= m_ref_horz.select<8,1,8,1>(0,3);
    m_ref_bkp += 4;
    m_out = cm_shr<uchar>(m_ref_bkp,3,SAT);
}

void _GENX_ inline DHpelInterp(SurfaceIndex SURF_REF,
    short xref, short yref, matrix<uchar, 8, 8> &m_out) {

    matrix<uchar, 11, 16> m_ref_diag;
    matrix<short, 11, 8> m_ref_bkp;
    matrix<short, 8, 8> m_ref_bkp2;
    read_plane_ext<16, 11>(SURF_REF, xref - 1, yref - 1, m_ref_diag);
    m_ref_bkp  = (m_ref_diag.select<11,1,8,1>(0,1) + m_ref_diag.select<11,1,8,1>(0,2));
    m_ref_bkp *= 5;
    m_ref_bkp -= m_ref_diag.select<11,1,8,1>(0,0);
    m_ref_bkp -= m_ref_diag.select<11,1,8,1>(0,3);

    m_ref_bkp2  = (m_ref_bkp.select<8,1,8,1>(1,0) + m_ref_bkp.select<8,1,8,1>(2,0));
    m_ref_bkp2 *= 5;
    m_ref_bkp2 -= m_ref_bkp.select<8,1,8,1>(0,0);
    m_ref_bkp2 -= m_ref_bkp.select<8,1,8,1>(3,0);
    m_ref_bkp2 += 32;
    m_out = cm_shr<uchar>(m_ref_bkp2,6,SAT);
}

enum _HHpelPos {
    _a,
    _b
};

void _GENX_ inline abQpelInterp(SurfaceIndex SURF_REF,
    short xref, short yref, _HHpelPos pos, matrix<uchar, 8, 8> &m_out) {
    matrix<uchar, 8, 8> m_ref;
    matrix<uchar, 8, 16> m_ref_horz;
    matrix<short, 8, 8> m_ref_bkp;
    read_plane_ext<16, 8>(SURF_REF, xref - 1, yref, m_ref_horz);
    m_ref_bkp  = (m_ref_horz.select<8,1,8,1>(0,1) + m_ref_horz.select<8,1,8,1>(0,2));
    m_ref_bkp *= 5;
    m_ref_bkp -= m_ref_horz.select<8,1,8,1>(0,0);
    m_ref_bkp -= m_ref_horz.select<8,1,8,1>(0,3);
    m_ref_bkp += 4;
    m_ref = cm_shr<uchar>(m_ref_bkp,3,SAT);
    m_out = cm_avg<uchar>(m_ref_horz.select<8,1,8,1>(0, 1 + pos),m_ref);
}

enum _VHpelPos {
    _c,
    _i
};

void _GENX_ inline ciQpelInterp(SurfaceIndex SURF_REF,
    short xref, short yref, _VHpelPos pos, matrix<uchar, 8, 8> &m_out) {

    matrix<uchar, 8, 8> m_ref;
    matrix<unsigned char, 11, 8> m_ref_vert;
    matrix<short, 8, 8> m_ref_bkp;
    read_plane_ext<8, 11>(SURF_REF, xref, yref - 1, m_ref_vert);
    m_ref_bkp  = (m_ref_vert.select<8,1,8,1>(1,0) + m_ref_vert.select<8,1,8,1>(2,0));
    m_ref_bkp *= 5;
    m_ref_bkp -= m_ref_vert.select<8,1,8,1>(0,0);
    m_ref_bkp -= m_ref_vert.select<8,1,8,1>(3,0);
    m_ref_bkp += 4;
    m_ref = cm_shr<uchar>(m_ref_bkp,3,SAT);
    m_out = cm_avg<uchar>(m_ref_vert.select<8,1,8,1>(1 + pos, 0),m_ref);
}

void _GENX_ inline dfjlQpelInterp(SurfaceIndex SURF_REF,
    short xref, short yref, _VHpelPos yoff, _HHpelPos xoff, matrix<uchar, 8, 8> &m_out) {

    matrix<uchar, 8, 8> m_ref;
    matrix<uchar, 11, 16> m_ref_diag;
    matrix<short, 11, 8> m_ref_bkp;
    matrix<short, 8, 8> m_ref_bkp2;
    read_plane_ext<16, 11>(SURF_REF, xref - 1, yref - 1, m_ref_diag);
    m_ref_bkp  = (m_ref_diag.select<11,1,8,1>(0,1) + m_ref_diag.select<11,1,8,1>(0,2));
    m_ref_bkp *= 5;
    m_ref_bkp -= m_ref_diag.select<11,1,8,1>(0,0);
    m_ref_bkp -= m_ref_diag.select<11,1,8,1>(0,3);

    m_ref_bkp2  = (m_ref_bkp.select<8,1,8,1>(1,0) + m_ref_bkp.select<8,1,8,1>(2,0));
    m_ref_bkp2 *= 5;
    m_ref_bkp2 -= m_ref_bkp.select<8,1,8,1>(0,0);
    m_ref_bkp2 -= m_ref_bkp.select<8,1,8,1>(3,0);
    m_ref_bkp2 += 32;
    m_ref = cm_shr<uchar>(m_ref_bkp2,6,SAT);
    m_out = cm_avg<uchar>(m_ref_diag.select<8,1,8,1>(1 + yoff, 1 + xoff),m_ref);
}

void _GENX_ inline ekQpelInterp(SurfaceIndex SURF_REF,
    short xref, short yref, _VHpelPos yoff, matrix<uchar, 8, 8> &m_out) {

    matrix<uchar, 8, 8> m_ref_v;
    matrix<uchar, 8, 8> m_ref_d;

    HHpelInterp(SURF_REF, xref, yref + yoff, m_ref_v);
    DHpelInterp(SURF_REF, xref, yref, m_ref_d);

    m_out = cm_avg<uchar>(m_ref_v,m_ref_d);
}

void _GENX_ inline ghQpelInterp(SurfaceIndex SURF_REF,
    short xref, short yref, _HHpelPos xoff, matrix<uchar, 8, 8> &m_out) {

    matrix<uchar, 8, 8> m_ref_h;
    matrix<uchar, 8, 8> m_ref_d;

    VHpelInterp(SURF_REF, xref + xoff, yref, m_ref_h);
    DHpelInterp(SURF_REF, xref, yref, m_ref_d);

    m_out = cm_avg<uchar>(m_ref_h,m_ref_d);
}

void _GENX_ inline BlockSPel(SurfaceIndex SURF_REF,int x, int y,
                              vector< short, 2 > mv, matrix<uchar, 8, 8> &m_ref) {
        
    int x0 = x,
        y0 = y;

    int xref0 = (x0 + mv[0]/4),
        yref0 = (y0 + mv[1]/4);
    int xrem = mv[0] % 4,
        yrem = mv[1] % 4;

    int val = mv[0] < 0;
    int cor = xrem  < 0;
    xrem = (((xrem + 4) * val) + (xrem * !val)) * (xrem != 0);
    xref0 += (-1 * val) * cor;
    val = mv[1] < 0;
    cor = yrem < 0;
    yrem = (((yrem + 4) * val) + (yrem * !val)) * (yrem != 0);
    yref0 += (-1 *val) * cor;
    
    if (yrem == 0) {
        if(xrem == 0)
            read_plane(SURF_REF, GENX_SURFACE_Y_PLANE, xref0, yref0, m_ref);
        else if (xrem == 1)
            abQpelInterp(SURF_REF, xref0, yref0, _a, m_ref);
        else if (xrem == 2)
            HHpelInterp(SURF_REF, xref0, yref0, m_ref);
        else
            abQpelInterp(SURF_REF, xref0, yref0, _b, m_ref);
    } else if (yrem == 1) {
        if (xrem == 0)
            ciQpelInterp(SURF_REF, xref0, yref0, _c, m_ref);
        else if (xrem == 1)
            dfjlQpelInterp(SURF_REF, xref0, yref0, _c, _a, m_ref);
        else if (xrem == 2)
            ekQpelInterp(SURF_REF, xref0, yref0, _c, m_ref);
        else
            dfjlQpelInterp(SURF_REF, xref0, yref0, _c, _b, m_ref);
    } else if (yrem == 2) {
        if (xrem == 0)
            VHpelInterp(SURF_REF, xref0, yref0, m_ref);
        else if (xrem == 1)
            ghQpelInterp(SURF_REF, xref0, yref0, _a, m_ref);
        else if (xrem == 2)
            DHpelInterp(SURF_REF, xref0, yref0, m_ref);
        else
            ghQpelInterp(SURF_REF, xref0, yref0, _b, m_ref);
    } else {
        if (xrem == 0)
            ciQpelInterp(SURF_REF, xref0, yref0, _i, m_ref);
        else if (xrem == 1)
            dfjlQpelInterp(SURF_REF, xref0, yref0, _i, _a, m_ref);
        else if (xrem == 2)
            ekQpelInterp(SURF_REF, xref0, yref0, _i, m_ref);
        else
            dfjlQpelInterp(SURF_REF, xref0, yref0, _i, _b, m_ref);
    }
}

//inline _GENX_
//vector<float, 2> Genx_RsCs_aprox_8x8Block(matrix<uchar, 8, 8> rc) {
//	vector<uint, 2>RsCs = 0;
//	vector<float, 2>RsCsT = 0.0;
//
//	matrix<int, 7,8> temp = 0;
//	temp = rc.select<7,1,8,1>(0,0) - rc.select<7,1,8,1>(1,0);
//	temp = temp * temp;
//	RsCs.format<uint>()[0] = cm_sum<uint>(temp) >> 5;
//	matrix<int, 8,7> temp2 = 0;
//	temp2 = rc.select<8,1,7,1>(0,0) - rc.select<8,1,7,1>(0,1);
//	temp2 = temp2 * temp2;
//	RsCs.format<uint>()[1] = cm_sum<uint>(temp2) >> 5;
//
//	RsCsT.format<float>()[0] = (float)RsCs.format<uint>()[0];
//	RsCsT.format<float>()[1] = (float)RsCs.format<uint>()[1];
//	RsCsT = cm_sqrt<2>(RsCsT);
//	return RsCsT;
//}

inline _GENX_
vector<float, 2> Genx_RsCs_aprox_8x8Block(matrix<uchar, 8, 8> rc) {
	vector<uint, 2>RsCs = 0;
	vector<float, 2>RsCsT = 0.0;

	matrix<int, 4, 4> temp = 0;
	temp = rc.select<4, 1, 4, 1>(2, 2) - rc.select<4, 1, 4, 1>(3, 2);
	temp = temp * temp;
	RsCs.format<uint>()[0] = cm_sum<uint>(temp) >> 4;
	matrix<int, 4, 4> temp2 = 0;
	temp2 = rc.select<4, 1, 4, 1>(2, 2) - rc.select<4, 1, 4, 1>(2, 3);
	temp2 = temp2 * temp2;
	RsCs.format<uint>()[1] = cm_sum<uint>(temp2) >> 4;

	RsCsT.format<float>()[0] = (float)RsCs.format<uint>()[0];
	RsCsT.format<float>()[1] = (float)RsCs.format<uint>()[1];
	RsCsT = cm_sqrt<2>(RsCsT);
	return RsCsT;
}

inline _GENX_
matrix<uchar, 8, 8> Genx_OMC_8x8Block(SurfaceIndex SURF_REF, matrix<short,2,4> mv, int x, int y, uchar rc0, uchar rc1, uchar rc2, uchar rc3/*, vector<float, 2> *RsCs*/) {
	matrix<uchar, 8, 8>
		r0   = 0,
		r1   = 0,
		r2   = 0,
		r3   = 0,
		rout = 0;
	uchar
		q_val  = 0;

	if(rc0)
		read_plane(SURF_REF, GENX_SURFACE_Y_PLANE, x + mv.row(0).select<2, 1>(0)[0], y + mv.row(0).select<2, 1>(0)[1], r0);
	if(rc1)
		read_plane(SURF_REF, GENX_SURFACE_Y_PLANE, x + mv.row(0).select<2, 1>(2)[0], y + mv.row(0).select<2, 1>(2)[1], r1);
	if(rc2)
		read_plane(SURF_REF, GENX_SURFACE_Y_PLANE, x + mv.row(1).select<2, 1>(0)[0], y + mv.row(1).select<2, 1>(0)[1], r2);
	if(rc3)
		read_plane(SURF_REF, GENX_SURFACE_Y_PLANE, x + mv.row(1).select<2, 1>(2)[0], y + mv.row(1).select<2, 1>(2)[1], r3);


	q_val = rc0 + rc1 + rc2 + rc3;
//	rout = ((r0 * rc0) + (r1 * rc1) + (r2 * rc2) + (r3 * rc3) + (q_val >> 1)) / (q_val);
	rout = ((r0 + r1 + r2 + r3) + (q_val >> 1)) / (q_val);
	return rout;
}

inline _GENX_
#if PRINTDEBUG
int4 SimIdx_8x8p(matrix<uchar, 8, 8> T1, matrix<uchar, 8, 8> T2, short th_val, int4 size, vector<float, 2> RsCsDiff, uint threadId) {
#else
int4 SimIdx_8x8p(matrix<uchar, 8, 8> T1, matrix<uchar, 8, 8> T2, short th_val, int4 size, vector<float, 2> RsCsDiff) {
#endif
	short val_s = cm_sum<short>(cm_abs<short>(T1 - T2));
#if PRINTDEBUG
	printf("%d\tSAD: %d\tsize: %d\t", threadId, val_s, size);
#endif
	int4 val = (val_s * val_s);// + ((RsCsDiff(0) * RsCsDiff(0)) + (RsCsDiff(1) * RsCsDiff(1)));
	float size_f = size;
	short th_origin = th_val;
	/*if (RsCsDiff(0) < 0 && RsCsDiff(1) < 0)
		return 0;*/
		//th_origin = th_val / 2;
	int4 th = th_origin * th_origin / ((cm_sqrt(size_f + ((RsCsDiff(0) * RsCsDiff(0)) + (RsCsDiff(1) * RsCsDiff(1)))) / 16) + 1);// / ((cm_sqrt(size_f * ((RsCsDiff(0) * RsCsDiff(0)) + (RsCsDiff(1) * RsCsDiff(1)))) / 16) + 1);
	if(th <= val || val > 83968)
		return 0;
	int4 sub  = th - val;
	int4 sum  = th + val;
	int4 sel1 = sub < SELECTION_THRESHOLD;
	int4 sel2 = !sel1;
	int4 ssub = ((sub << WEIGHT_MULTIPLIER) / sum) * sel1;
	int4 ssum = (sub / (sum >> WEIGHT_MULTIPLIER)) * sel2;
	int4 wfactor = (ssub + ssum);
	return wfactor;
}

inline _GENX_
matrix<uchar, 8, 8> MedianIdx_8x8_3ref(matrix<uchar, 8, 8> T1, matrix<uchar, 8, 8> T2, matrix<uchar, 8, 8> T3) {
	matrix<uchar, 8, 8>
        mean = 0;
	mean.format<uchar>() = cm_max<uchar>(cm_min<uchar>(T1.format<uchar>(),T3.format<uchar>()), T2.format<uchar>());
	mean.format<uchar>() = cm_min<uchar>(cm_max<uchar>(T1.format<uchar>(),T3.format<uchar>()), mean.format<uchar>());
	return mean;
}

inline _GENX_
matrix<ushort, 8, 8>OMC_Ref_Generation(SurfaceIndex SURF_REF, SurfaceIndex SURF_MV, ushort width, ushort height, uint mbX, uint mbY, short noSC, matrix<short,2,4> &mv) {
    matrix<short,2,4>
        mv8_g4 = 0,
        mv8_g1 = 0;
    uint
        x    = mbX * 8,
        y    = mbY * 8,
        mv_x = mbX * 4,
        mv_y = mbY;
    int1
        x_o  = !(x % (width - 8)) << 2,
        y_o  = !(y % (height - 1));
    ushort
        picWidthInMB  = (width >> 3) - 1,
        picHeightInMB = (height >> 3) - 1;
    uchar 
        rc0  = (mbX < picWidthInMB && mbY < picHeightInMB),
        rc1  = (mbX > 0            && mbY < picHeightInMB),
        rc2  = (mbX < picWidthInMB && mbY > 0),
        rc3  = (mbX > 0            && mbY > 0);
    read_plane(SURF_MV, GENX_SURFACE_Y_PLANE, mv_x - 4 + x_o, mv_y - 1 + y_o, mv8_g4);
    mv8_g1 = mv8_g4 / 4;
    mv = (mv8_g1 * mv8_g1) * noSC;
    return(Genx_OMC_8x8Block(SURF_REF, mv8_g1, x, y, rc0, rc1, rc2, rc3));
}

inline _GENX_
#if PRINTDEBUG
int4 mergeStrengthCalculator(matrix<uchar, 8, 8> refBlock, matrix<uchar, 12, 16> src, vector<float, 2> RsCsT, short noSC, ushort th, int4 size, uint threadId) {
#else
int4 mergeStrengthCalculator(matrix<uchar, 8, 8> refBlock, matrix<uchar, 12, 16> src, vector<float, 2> RsCsT, short noSC, ushort th, int4 size) {
#endif
	vector<float, 2>
		RsCsRef = Genx_RsCs_aprox_8x8Block(refBlock);
#if PRINTDEBUG
	printf("%d\tRefRs: %.4lf\tRefCs: %.4lf\t", threadId, RsCsRef(0), RsCsRef(1));
	if (noSC)
		return(SimIdx_8x8p(refBlock, src.select<8, 1, 8, 1>(2, 2), th, size, RsCsT - RsCsRef, threadId)); //check
#else
	if (noSC)
		return(SimIdx_8x8p(refBlock, src.select<8, 1, 8, 1>(2, 2), th, size, RsCsT - RsCsRef)); //check
#endif
	else
		return 0;
}

inline _GENX_
int4 mergeStrengthCalculator(matrix<uchar, 8, 8> refBlock, matrix<uchar, 12, 12> src, vector<float, 2> RsCsT, short noSC, ushort th, int4 size) {
    vector<float, 2>
        RsCsRef = Genx_RsCs_aprox_8x8Block(refBlock);
    if(noSC)
#if PRINTDEBUG
        return(SimIdx_8x8p(refBlock, src.select<8,1,8,1>(2,2), th, size, RsCsT - RsCsRef,0)); //check
#else
		return(SimIdx_8x8p(refBlock, src.select<8, 1, 8, 1>(2, 2), th, size, RsCsT - RsCsRef)); //check
#endif
    else
        return 0;
}

inline _GENX_
int4 mergeStrengthCalculator(matrix<uchar, 8, 8> refBlock, matrix<uchar, 8, 8> src, vector<float, 2> RsCsT, short noSC, ushort th, int4 size) {
	vector<float, 2>
		RsCsRef = Genx_RsCs_aprox_8x8Block(refBlock);
	if (noSC)
#if PRINTDEBUG
		return(SimIdx_8x8p(refBlock, src, th, size, RsCsT - RsCsRef,0)); //check
#else
		return(SimIdx_8x8p(refBlock, src, th, size, RsCsT - RsCsRef)); //check
#endif
	else
		return 0;
}

inline _GENX_
#if PRINTDEBUG
matrix<ushort, 8, 8>mergeBlocksRef(matrix<uchar, 12, 16> src,
	matrix<uchar, 8, 8> refBlock1, int4 similarityVal1, uint threadId) {
#else
matrix<ushort, 8, 8>mergeBlocksRef(matrix<uchar, 12, 16> src,
	matrix<uchar, 8, 8> refBlock1, int4 similarityVal1) {
#endif
	int4 norm = MERGE_LIMIT + 1 + similarityVal1;
	int4 w1 = similarityVal1 * MERGE_LIMIT / norm;
	int4 srcw = MERGE_LIMIT - w1;
#if PRINTDEBUG
	printf("%d\tW1: %d\tWSrc: %d\t", threadId, w1, srcw);
#endif
	return((src.select<8, 1, 8, 1>(2, 2) * srcw + refBlock1 * w1 + 128) >> 8);
}

inline _GENX_
matrix<ushort, 8, 8>mergeBlocksRef(matrix<uchar, 12, 12> src,
                                    matrix<uchar, 8, 8> refBlock1, int4 similarityVal1) {
    int4 norm = MERGE_LIMIT + 1 + similarityVal1;
    int4 w1 = similarityVal1 * MERGE_LIMIT / norm;
    int4 srcw = MERGE_LIMIT - w1;
    return((src.select<8,1,8,1>(2,2) * srcw + refBlock1 * w1 + 128) >> 8);
}

inline _GENX_
matrix<ushort, 8, 8>mergeBlocksRef(matrix<uchar, 8, 8> src,
	matrix<uchar, 8, 8> refBlock1, int4 similarityVal1) {
	int4 norm = MERGE_LIMIT + 1 + similarityVal1;
	int4 w1 = similarityVal1 * MERGE_LIMIT / norm;
	int4 srcw = MERGE_LIMIT - w1;
	return((src * srcw + refBlock1 * w1 + 128) >> 8);
}

inline _GENX_
#if PRINTDEBUG
matrix<ushort, 8, 8>mergeBlocks2Ref(matrix<uchar, 12, 16> src,
	matrix<uchar, 8, 8> refBlock1, int4 similarityVal1,
	matrix<uchar, 8, 8> refBlock2, int4 similarityVal2, uint threadId) {
#else
matrix<ushort, 8, 8>mergeBlocks2Ref(matrix<uchar, 12, 16> src,
	matrix<uchar, 8, 8> refBlock1, int4 similarityVal1,
	matrix<uchar, 8, 8> refBlock2, int4 similarityVal2) {
#endif
	int4 norm = MERGE_LIMIT + 1 + similarityVal1 + similarityVal2;
	int4 w1 = similarityVal1 * MERGE_LIMIT / norm;
	int4 w2 = similarityVal2 * MERGE_LIMIT / norm;
	int4 srcw = MERGE_LIMIT - w1 - w2;
#if PRINTDEBUG
	printf("%d\tW1: %d\tW2: %d\tWSrc: %d\t", threadId, w1, w2, srcw);
#endif
	return((src.select<8, 1, 8, 1>(2, 2) * srcw + refBlock1 * w1 + refBlock2 * w2 + 128) >> 8);
}

inline _GENX_
matrix<ushort, 8, 8>mergeBlocks2Ref(matrix<uchar, 12, 12> src,
                                    matrix<uchar, 8, 8> refBlock1, int4 similarityVal1,
                                    matrix<uchar, 8, 8> refBlock2, int4 similarityVal2) {
    int4 norm = MERGE_LIMIT + 1 + similarityVal1 + similarityVal2;
    int4 w1 = similarityVal1 * MERGE_LIMIT / norm;
    int4 w2 = similarityVal2 * MERGE_LIMIT / norm;
    int4 srcw = MERGE_LIMIT - w1 - w2;
    return((src.select<8,1,8,1>(2,2) * srcw + refBlock1 * w1 + refBlock2 * w2 + 128) >> 8);
}

inline _GENX_
matrix<ushort, 8, 8>mergeBlocks2Ref(matrix<uchar, 8, 8> src,
	matrix<uchar, 8, 8> refBlock1, int4 similarityVal1,
	matrix<uchar, 8, 8> refBlock2, int4 similarityVal2) {
	int4 norm = MERGE_LIMIT + 1 + similarityVal1 + similarityVal2;
	int4 w1 = similarityVal1 * MERGE_LIMIT / norm;
	int4 w2 = similarityVal2 * MERGE_LIMIT / norm;
	int4 srcw = MERGE_LIMIT - w1 - w2;
	return((src * srcw + refBlock1 * w1 + refBlock2 * w2 + 128) >> 8);
}