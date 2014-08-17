#pragma once

#include "smmintrin.h"
#include "xmmintrin.h"
#include "emmintrin.h"

typedef __m128 vec;

struct mat
{
	vec x;
	vec y;
	vec z;
	vec w;
};



inline
vec vec_cast(__m128d v)
{
	return _mm_castpd_ps(v);
}
inline
vec vec_cast(__m128i v)
{
	return _mm_castsi128_ps(v);
}

inline vec vsetw(vec v, float f)
{
	vec r = v;
#ifdef _WIN32
	r.m128_f32[3] = f;
#else
	r[3] = f;
#endif
	return r;
}



inline vec vsetxzero(vec v)
{
	vec r = v;
#ifdef _WIN32
	r.m128_f32[0] = 0.f;
#else
	r[0] = 0.f;
#endif
	return r;
}
inline vec vsetyzero(vec v)
{
	vec r = v;
#ifdef _WIN32
	r.m128_f32[1] = 0.f;
#else
	r[1] = 0.f;
#endif
	return r;
}
inline vec vsetzzero(vec v)
{
	vec r = v;
#ifdef _WIN32
	r.m128_f32[2] = 0.f;
#else
	r[2] = 0.f;
#endif

	return r;
}

inline vec vsetwzero(vec v)
{
	vec r = v;
#ifdef _WIN32
	r.m128_f32[3] = 0.f;
#else
	r[3] = 0.f;
#endif

	return r;
}


inline float vgetx(vec v)
{
#ifdef _WIN32
	return v.m128_f32[0];
#else
	return v[0];
#endif
}
inline float vgety(vec v)
{
#ifdef _WIN32
	return v.m128_f32[1];
#else
	return v[1];
#endif
}
inline float vgetz(vec v)
{
#ifdef _WIN32
	return v.m128_f32[2];
#else
	return v[2];
#endif
}
inline float vgetw(vec v)
{
#ifdef _WIN32
	return v.m128_f32[3];
#else
	return v[3];
#endif
}

inline mat mload44(void* p)
{
	mat r;
	float* pf = (float*)p;
	r.x = _mm_loadu_ps(pf);
	r.y = _mm_loadu_ps(pf + 4);
	r.z = _mm_loadu_ps(pf + 8);
	r.w = _mm_loadu_ps(pf + 12);
	return r;
}

inline vec vload1(void* p)
{
	vec r = _mm_load_ss((float*)p);
	return r;
}

inline vec vload2(void* p)
{
	vec r = vec_cast(_mm_load_sd((double*)p));
	return r;
}

inline vec vload3(void* p)
{
	vec r = _mm_loadu_ps((float*)p);
	return r;
}
inline vec vload4(void* p)
{
	vec r = _mm_loadu_ps((float*)p);
	return r;

}


inline void vstore4(void* pDst, vec v)
{
	_mm_storeu_ps((float*)pDst, v);
}

inline void vstore3(void* p, vec v)
{
	//v = _mm_and_ps(v, (vec)_mm_set_epi32(0,-1,-1,-1));
	_mm_maskmoveu_si128(_mm_castps_si128(v), (__m128i)_mm_set_epi32(0,-1,-1,-1), (char*)p);
	//_mm_storeu_ps((float*)p, v);
}

inline void vstore2(void* p, vec v)
{
	//v = _mm_and_ps(v, (vec)_mm_set_epi32(0,-1,-1,-1));
	_mm_store_sd((double*)p, _mm_castps_pd(v));
	//_mm_storeu_ps((float*)p, v);
}
inline void vstore1(void* p, vec v)
{
	//v = _mm_and_ps(v, (vec)_mm_set_epi32(0,-1,-1,-1));
	_mm_store_ss((float*)p, v);
	//_mm_storeu_ps((float*)p, v);
}

inline vec vadd(vec a, vec b)
{
	return _mm_add_ps(a,b);
}

inline vec vsub(vec a, vec b)
{
	return _mm_sub_ps(a,b);
}

inline vec vmul(vec a, vec b)
{
	return _mm_mul_ps(a,b);
}

inline vec vsplatx(vec v)
{
	return _mm_shuffle_ps(v, v, _MM_SHUFFLE(0,0,0,0));
}

inline vec vsplaty(vec v)
{
	return _mm_shuffle_ps(v, v, _MM_SHUFFLE(1,1,1,1));
}

inline vec vsplatz(vec v)
{
	return _mm_shuffle_ps(v, v, _MM_SHUFFLE(2,2,2,2));
}
inline vec vsplatw(vec v)
{
	return _mm_shuffle_ps(v, v, _MM_SHUFFLE(3,3,3,3));
}

inline vec vrep(float f)
{
	return _mm_set1_ps(f);
}

inline vec vneg(vec v)
{
	return _mm_xor_ps(v, vec_cast(_mm_set1_epi32(0x80000000)));
}
inline vec vrsqrt(vec x)
{
	vec v = _mm_rsqrt_ps(x);
	vec halfx = _mm_mul_ps(x, _mm_set1_ps(-0.5f));
	vec x2 = _mm_mul_ps(v, v);
	vec foo = _mm_mul_ps(v, _mm_add_ps(_mm_set1_ps(1.5f), _mm_mul_ps(x2, halfx)));
	return foo;
}

inline vec vnormalize3(vec v)
{
	v = vsetwzero(v);
	vec r0 = _mm_mul_ps(v, v);
	vec r1 = _mm_hadd_ps(r0, r0);
	vec r2 = _mm_hadd_ps(r1, r1);
	vec result = _mm_mul_ps(vrsqrt(r2), v);
	return result;


}
inline vec vcross3(vec l, vec r)
{
	// r.x = v0.y * v1.z - v0.z * v1.y;
	// r.y = v0.z * v1.x - v0.x * v1.z;
	// r.z = v0.x * v1.y - v0.y * v1.x;
	vec v0yzx = _mm_shuffle_ps(l, l, _MM_SHUFFLE(0, 0, 2, 1));
	vec v1zxy = _mm_shuffle_ps(r, r, _MM_SHUFFLE(0, 1, 0, 2));
	vec v0zxy = _mm_shuffle_ps(l, l, _MM_SHUFFLE(0, 1, 0, 2));
	vec v1yzx = _mm_shuffle_ps(r, r, _MM_SHUFFLE(0, 0, 2, 1));
	return _mm_sub_ps(_mm_mul_ps(v0yzx, v1zxy), _mm_mul_ps(v0zxy, v1yzx));
}
inline vec vdot3(vec l, vec r)
{
	l = vsetwzero(l);
	vec r0 = _mm_mul_ps(l, r);
	vec r1 = _mm_hadd_ps(r0, r0);
	vec r2 = _mm_hadd_ps(r1, r1);
	return r2;

}
inline vec vdot4(vec l, vec r)
{
	vec r0 = _mm_mul_ps(l, r);
	vec r1 = _mm_hadd_ps(r0, r0);
	vec r2 = _mm_hadd_ps(r1, r1);
	return r2;
}
#define vset(x,y,z,w) _mm_set_ps(w,z,y,x)

inline vec vinit31(vec v, float w)
{
	vec r = _mm_insert_ps(v, _mm_loadu_ps(&w), 0x30);
	return r;
}
inline
mat mmult(const mat& m0_, const mat& m1_)
{
	vec m0_x = m0_.x;
	vec m0_y = m0_.y;
	vec m0_z = m0_.z;
	vec m0_w = m0_.w;
	vec m1_x = m1_.x;
	vec m1_y = m1_.y;
	vec m1_z = m1_.z;
	vec m1_w = m1_.w;

	vec rx = _mm_mul_ps(m0_x, _mm_shuffle_ps(m1_x, m1_x, 0));
	rx = _mm_add_ps(rx, _mm_mul_ps(m0_y, _mm_shuffle_ps(m1_x,m1_x, 0x55)));
	rx = _mm_add_ps(rx, _mm_mul_ps(m0_z, _mm_shuffle_ps(m1_x, m1_x, 0xaa)));
	rx = _mm_add_ps(rx, _mm_mul_ps(m0_w, _mm_shuffle_ps(m1_x, m1_x, 0xff)));
	vec ry = _mm_mul_ps(m0_x, _mm_shuffle_ps(m1_y, m1_y, 0));
	ry = _mm_add_ps(ry, _mm_mul_ps(m0_y, _mm_shuffle_ps(m1_y, m1_y, 0x55)));
	ry = _mm_add_ps(ry, _mm_mul_ps(m0_z, _mm_shuffle_ps(m1_y, m1_y, 0xaa)));
	ry = _mm_add_ps(ry, _mm_mul_ps(m0_w, _mm_shuffle_ps(m1_y, m1_y, 0xff)));
	vec rz = _mm_mul_ps(m0_x, _mm_shuffle_ps(m1_z, m1_z, 0));
	rz = _mm_add_ps(rz, _mm_mul_ps(m0_y, _mm_shuffle_ps(m1_z, m1_z, 0x55)));
	rz = _mm_add_ps(rz, _mm_mul_ps(m0_z, _mm_shuffle_ps(m1_z, m1_z, 0xaa)));
	rz = _mm_add_ps(rz, _mm_mul_ps(m0_w, _mm_shuffle_ps(m1_z, m1_z, 0xff)));
	vec rw = _mm_mul_ps(m0_x, _mm_shuffle_ps(m1_w, m1_w, 0));
	rw = _mm_add_ps(rw, _mm_mul_ps(m0_y, _mm_shuffle_ps(m1_w,m1_w, 0x55)));
	rw = _mm_add_ps(rw, _mm_mul_ps(m0_z, _mm_shuffle_ps(m1_w, m1_w, 0xaa)));
	rw = _mm_add_ps(rw, _mm_mul_ps(m0_w, _mm_shuffle_ps(m1_w, m1_w, 0xff)));

	mat r;
	r.x = rx;
	r.y = ry;
	r.z = rz;
	r.w = rw;
	return r;
}

inline
vec obbtoaabb(mat mrotation, vec vHalfSize)
{
	vec AbsMask = _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff));
	vec SizeX = _mm_shuffle_ps(vHalfSize, vHalfSize, _MM_SHUFFLE(0,0,0,0));
	vec SizeY = _mm_shuffle_ps(vHalfSize, vHalfSize, _MM_SHUFFLE(1,1,1,1));
	vec SizeZ = _mm_shuffle_ps(vHalfSize, vHalfSize, _MM_SHUFFLE(2,2,2,2));
	vec vx = _mm_and_ps(_mm_mul_ps(mrotation.x, SizeX), AbsMask);
	vec vy = _mm_and_ps(_mm_mul_ps(mrotation.y, SizeY), AbsMask);
	vec vz = _mm_and_ps(_mm_mul_ps(mrotation.z, SizeZ), AbsMask);
	return _mm_add_ps(_mm_add_ps(vx,vy),vz);
}


inline float tripleproduct(vec plane0, vec plane1, vec ptest)
{
	vec xp = vcross3(plane0, plane1);
	vec r = vdot3(ptest, xp);
	return vgetx(r);
}

vec vmakeplane(vec p, vec normal)
{

	vec fDot = vdot3(normal,p);
	vec fNegDot = _mm_xor_ps(fDot, _mm_castsi128_ps(_mm_set1_epi32(0x80000000))); //neg
	//vec r = _mm_insert_ps(normal, fNegDot, 0x30);
	vec r = normal;//_mm_insert_ps(normal, fNegDot, 0x30);
	r = vsetw(r, vgetx(fNegDot));
	return r;
}

inline mat minverserotation(mat m)
{
	mat r;
	// r.x.z = mat.z.x;
	// r.z.x = mat.x.z;

	// r.x.y = mat.y.x;
	// r.y.x = mat.x.y;

	// r.y.z = mat.z.y;
	// r.z.y = mat.y.z;
	vec x = m.x;
	vec y = m.y;
	vec z = m.z;

	vec zx_zy_zz = _mm_shuffle_ps(z, z, _MM_SHUFFLE(0, 2, 1, 0));

	vec xx_oo_yx = _mm_shuffle_ps(x, y, _MM_SHUFFLE(0,0,0,0));
	vec xy_oo_yy = _mm_shuffle_ps(x, y, _MM_SHUFFLE(0,1,0,1));
	vec xz_oo_yz = _mm_shuffle_ps(x, y, _MM_SHUFFLE(0,2,0,2));

	vec xx_yx_zx = _mm_shuffle_ps(xx_oo_yx, zx_zy_zz, _MM_SHUFFLE(0,0,2,0));
	vec xy_yy_zy = _mm_shuffle_ps(xy_oo_yy, zx_zy_zz, _MM_SHUFFLE(0,1,2,0));
	vec xz_yz_zz = _mm_shuffle_ps(xz_oo_yz, zx_zy_zz, _MM_SHUFFLE(0,2,2,0));

	r.x = xx_yx_zx;
	r.y = xy_yy_zy; // xz yz zz
	r.z = xz_yz_zz; // xz yz zz
	r.w = m.w;
	return r;

}

inline
vec mrotate(mat m, vec pos)
{
	// r.x = m.x.x * point.x + m.y.x * point.y + m.z.x * point.z;
	// r.y = m.x.y * point.x + m.y.y * point.y + m.z.y * point.z;
	// r.z = m.x.z * point.x + m.y.z * point.y + m.z.z * point.z;
	vec rx = _mm_mul_ps(m.x, _mm_shuffle_ps(pos, pos, _MM_SHUFFLE(0,0,0,0)));
	vec ry = _mm_mul_ps(m.y, _mm_shuffle_ps(pos, pos, _MM_SHUFFLE(1,1,1,1)));
	vec rz = _mm_mul_ps(m.z, _mm_shuffle_ps(pos, pos, _MM_SHUFFLE(2,2,2,2)));
	return _mm_add_ps(rx, _mm_add_ps(ry, rz));
}

mat maffineinverse(mat m)
{
	// p = trans + rot * x
	// p - trans = rot * x
	// rot^ * (p - trans) = x
	// rot^ * p + rot^ * (-trans) = x

	mat mrot = minverserotation(m);
	vec trans = mrotate(mrot, vneg(m.w));//_mm_xor_ps(mat.w, _mm_set1_epi32(0x80000000)); //neg
	trans = vsetw(trans, 1.f);
	mrot.w = trans;//_mm_insert_ps(trans, _mm_set1_ps(1.f), 0x30);
	return mrot;
}
vec mtransformaffine(mat m, vec point)
{

	vec PX = _mm_shuffle_ps(point, point, _MM_SHUFFLE(0,0,0,0));
	vec PY = _mm_shuffle_ps(point, point, _MM_SHUFFLE(1,1,1,1));
	vec PZ = _mm_shuffle_ps(point, point, _MM_SHUFFLE(2,2,2,2));

	vec r = _mm_mul_ps(m.x, PX);
	r = _mm_add_ps(r, _mm_mul_ps(m.y, PY));
	r = _mm_add_ps(r, _mm_mul_ps(m.z, PZ));
	r = _mm_add_ps(r, m.w);


	return r;
	// v3 r;
	// r.x = mat.x.x * point.x + mat.y.x * point.y + mat.z.x * point.z;
	// r.y = mat.x.y * point.x + mat.y.y * point.y + mat.z.y * point.z;
	// r.z = mat.x.z * point.x + mat.y.z * point.y + mat.z.z * point.z;
	// r += v3init(mat.trans);
	// return r;
}

