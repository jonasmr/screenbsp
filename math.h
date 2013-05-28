#pragma once

#define PI 3.14159265358979323846f
#define TWOPI 6.2831853071795864769f
#define TORAD (PI/180.f)


struct v2;
struct v3;
struct v4;

struct v2
{
	float x;
	float y;

	void operator +=(const v2& r);
	void operator -=(const v2& r);
	void operator *=(const v2& r);
	void operator /=(const v2& r);
	void operator +=(const float r);
	void operator -=(const float r);
	void operator *=(const float r);
	void operator /=(const float r);

};



struct v3
{
	float x;
	float y;
	float z;
	void operator +=(const v3& r);
	void operator -=(const v3& r);
	void operator *=(const v3& r);
	void operator /=(const v3& r);

	void operator +=(const float r);
	void operator -=(const float r);
	void operator *=(const float r);
	void operator /=(const float r);

	v2 tov2();
	v4 tov4();
	v4 tov4(float w);


};
struct v4
{
	float x;
	float y;
	float z;
	float w;

	void operator +=(const v4& r);
	void operator -=(const v4& r);
	void operator *=(const v4& r);
	void operator /=(const v4& r);

	void operator +=(const float r);
	void operator -=(const float r);
	void operator *=(const float r);
	void operator /=(const float r);



	v2 tov2();
	v3 tov3();
};

union m
{
	struct
	{
		v4 x;
		v4 y;
		v4 z;
		union
		{
			v4 trans;
			v4 w;
		};
	};
};


v2 operator +(const v2 l, const v2 r);
v2 operator -(const v2 l, const v2 r);
v2 operator *(const v2 l, const v2 r);
v2 operator /(const v2 l, const v2 r);
bool operator <(const v2 l, const v2 r);

v2 operator +(const v2 l, float f);
v2 operator -(const v2 l, float f);
v2 operator *(const v2 v, float f);
v2 operator /(const v2 v, float f);
v2 operator +(float f, const v2 l);
v2 operator -(float f, const v2 l);
v2 operator *(float f, const v2 v);
v2 operator /(float f, const v2 v);

float v2length(v2 v);
v2 v2normalize(v2 v);
v2 v2hat(v2 v);
v2 v2reflect(v2 normal, v2 direction);
inline v2 v2neg(v2 v)
{ v.x = -v.x; v.y = -v.y; return v;}
float v2dot(v2 v);
float v2dot(v2 v0, v2 v1);
inline
float v2length2(v2 v){return v2dot(v,v);}
inline float v2distance2(v2 a, v2 b)
{
	return v2length2(a-b);
}
inline float v2distance(v2 a, v2 b)
{
	return v2length(a-b);
}

inline
v2 v2init(float x, float y){v2 r; r.x = x; r.y = y; return r;}
inline 
v2 v2init(float f){v2 r; r.x = f; r.y = f; return r;}

inline
v2 v2zero(){v2 z = {0,0};return z;}
inline
v2 v2max(v2 v0, v2 v1)
{
	v2 r; 
	r.x = v0.x < v1.x ? v1.x : v0.x;
	r.y = v0.y < v1.y ? v1.y : v0.y;
	return r; 
}
inline
v2 v2min(v2 v0, v2 v1)
{
	v2 r; 
	r.x = v0.x < v1.x ? v0.x : v1.x; 
	r.y = v0.y < v1.y ? v0.y : v1.y; 
	return r; 
}
v2 v2round(v2 v);
v2 v2sign(v2 v);
v2 v2abs(v2 v);
inline
void v2swap(v2& va, v2& vb) { v2 vtmp = vb; vb = va; va = vtmp; }
inline
v2 v2floor(v2 v){v2 r = v; r.x = floor(r.x); r.y = floor(r.y); return r;}
inline
v2 v2clamp(v2 value, v2 min_, v2 max_){ return v2min(max_, v2max(min_, value)); }
inline
v2 v2lerp(v2 from_, v2 to_, float fLerp) { return from_ + (to_-from_) * fLerp; }
inline
float v2ManhattanDistance(v2 from_, v2 to_) { return (fabs(from_.x-to_.x) + fabs(from_.y-to_.y)); }
inline
float fclamp(float value, float min_, float max_){ if(value<min_) return min_; if(value>max_) return max_; return value; }

float v2findpenetration(v2 pos, v2 dir, v2 size, v2 p0, v2 p1, v2 p2, v2 p3, v2& dirout, v2& pointout);
void v2createbounds(v2 vPosition, v2 vDirection, v2 vSize, v2& min_, v2& max_);
void v2createcorners(v2 vPosition, v2 vDirection, v2 vSize, v2& p0, v2& p1, v2& p2, v2& p3);
bool v2fucked(v2 v);
v2 v2fromangle(float fAngle);

inline bool v2iszero(v2 v){return v2length2(v)<1e-8f;}


v3 operator +(const v3 l, const v3 r);
v3 operator -(const v3 l, const v3 r);
v3 operator *(const v3 l, const v3 r);
v3 operator /(const v3 l, const v3 r);

v3 operator +(const v3 l, float f);
v3 operator -(const v3 l, float f);
v3 operator *(const v3 v, float f);
v3 operator /(const v3 v, float f);
v3 operator +(float f, const v3 l);
v3 operator -(float f, const v3 l);
v3 operator *(float f, const v3 v);
v3 operator /(float f, const v3 v);
v3 operator -(const v3 v);


inline 
v3 v3init(float f){v3 r; r.x = f; r.y = f; r.z = f; return r;}
inline
v3 v3init(float x, float y, float z){v3 r; r.x = x; r.y = y;r.z = z; return r;}
inline 
v3 v3init(v4 v)
{ v3 r; r.x = v.x; r.y = v.y; r.z = v.z; return r;}

inline
float v3dot(v3 v0, v3 v1)
{
	return v0.x * v1.x + v0.y * v1.y + v0.z * v1.z;
}
v3 v3cross(v3 v0, v3 v1);
float v3length(v3 v);
float v3lengthsq(v3 v);
v3 v3normalize(v3 v);
v3 v3min(v3 a, v3 b);
v3 v3min(v3 a, float f);
v3 v3max(v3 a, v3 b);
v3 v3max(v3 a, float f);

v3 v3abs(v3 a, v3 b);
v3 v3splatx(v3 v);
v3 v3splaty(v3 v);
v3 v3splatz(v3 v);
inline
v3 v3zero(){v3 z = {0,0,0};return z;}
inline 
v3 v3rep(float f){ v3 r = {f,f,f}; return r;}




inline 
v4 v4init(float f){v4 r; r.x = f; r.y = f; r.z = f; r.w = f; return r;}

inline
v4 v4init(float x, float y, float z, float w) {v4 r; r.x = x; r.y = y; r.z = z; r.w = w; return r;}
inline
v4 v4init(v3 v, float w){ v4 r; r.x = v.x; r.y = v.y; r.z = v.z; r.w = w; return r; }
inline
v4 v4init(v4 v, float w){ v4 r; r.x = v.x; r.y = v.y; r.z = v.z; r.w = w; return r; }
float v4dot(v4 v0, v4 v1);
float v4length(v4 v0);
float v4length2(v4 v0);


v4 operator +(const v4 l, const v4 r);
v4 operator -(const v4 l, const v4 r);
v4 operator *(const v4 l, const v4 r);
v4 operator /(const v4 l, const v4 r);

v4 operator +(const v4 l, float f);
v4 operator -(const v4 l, float f);
v4 operator *(const v4 v, float f);
v4 operator /(const v4 v, float f);
v4 operator +(float f, const v4 l);
v4 operator -(float f, const v4 l);
v4 operator *(float f, const v4 v);
v4 operator /(float f, const v4 v);
v4 operator -(const v4 v);



m minit(v3 vx, v3 vy, v3 vz, v3 vtrans);
m mid();
m mcreate(v3 vDir, v3 vRight, v3 vPoint);
m mmult(m m0, m m1);
m mrotatex(float fAngle);
m mrotatey(float fAngle);
m mrotatez(float fAngle);
m mtranslate(v3 trans);
m mviewport(float x, float y, float w, float h);
m mperspective(float fFovY, float fAspect, float fNear, float fFar);
m mortho(float fXWidth, float fYWidth, float fZRange);

v3 mtransform(m mat, v3 point);
v4 mtransform(m mat, v4 vector);
v3 mrotate(m mat, v3 vector);
m minverse(m);
void msetxaxis(m& mat, v3 axis);
void msetyaxis(m& mat, v3 axis);
void msetzaxis(m& mat, v3 axis);
inline v3 mgetxaxis(const m& mat){v3 r; r.x = mat.x.x; r.y = mat.x.y; r.z = mat.x.z; return r;}
inline v3 mgetyaxis(const m& mat){v3 r; r.x = mat.y.x; r.y = mat.y.y; r.z = mat.y.z; return r;}
inline v3 mgetzaxis(const m& mat){v3 r; r.x = mat.z.x; r.y = mat.z.y; r.z = mat.z.z; return r;}
m minverserotation(m mat);
m maffineinverse(m mat);
void ZASSERTAFFINE(m mat);


v3 obbtoaabb(m mrotation, v3 vHalfSize);
#define INTERSECT_FAIL (-FLT_MAX)
float rayplaneintersect(v3 r0, v3 rdir, v4 plane);
float rayplaneintersect(v3 r0, v3 rdir, v3 p0, v3 pnormal);
float rayboxintersect(v3 r0, v3 rdir, m boxtransform, v3 boxsize);


float frand();
int32_t randrange(int32_t nmin, int32_t nmax);
float frandrange(float fmin, float fmax);
v2 v2randir();
v2 v2randdisc();
uint32 randcolor();
uint32 randredcolor();

uint64_t rand64();
uint64_t rand64(uint64_t nPrev);

#define ZASSERTNORMALIZED2(v) ZASSERT(fabs(v2length(v)-1.f) < 1e-4f)
#define ZASSERTNORMALIZED3(v) ZASSERT(fabs(v3length(v)-1.f) < 1e-4f)
#define ZASSERTNORMALIZED4(v) ZASSERT(fabs(v4length(v)-1.f) < 1e-4f)

inline float signf(float f) { return f<0.0f ? -1.0f : 1.0f; }

#ifdef _WIN32
inline
float round(float f)
{
	return floor(f+0.5f);
}
#endif


template<typename T>
T Min(T a, T b)
{ return a < b ? a : b; }

template<typename T>
T Max(T a, T b)
{ return a > b ? a : b; }

template<typename T>
T Clamp(T v, T a, T b)
{ return Max(Min(v,b), a); }

template<typename T>
void Swap(T& a, T& b)
{
	T t = a;
	a = b;
	b = t;
}
