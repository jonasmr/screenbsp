#include <math.h>
#include <float.h>

#include "base.h"
#include "text.h"
#include "math.h"
#include "debug.h"


void v2::operator +=(const v2& r)
{
	x += r.x;
	y += r.y;
}
void v2::operator -=(const v2& r)
{
	x -= r.x;
	y -= r.y;
}
void v2::operator *=(const v2& r)
{
	x *= r.x;
	y *= r.y;
}
void v2::operator /=(const v2& r)
{
	x /= r.x;
	y /= r.y;
}
void v2::operator +=(const float r)
{
	x += r;
	y += r;
}
void v2::operator -=(const float r)
{
	x -= r;
	y -= r;
}
void v2::operator *=(const float r)
{
	x *= r;
	y *= r;
}
void v2::operator /=(const float r)
{
	x /= r;
	y /= r;
}

bool operator <(const v2 l, const v2 r)
{
	return (l.x<r.x && l.y<r.y);
}

v2 operator +(const v2 l, const v2 r)
{
	v2 res;
	res.x = l.x+r.x;
	res.y = l.y+r.y;
	return res;
}

v2 operator +(const v2 l, float f)
{
	v2 res;
	res.x = l.x+f;
	res.y = l.y+f;
	return res;
}

v2 operator +(float f, const v2 l)
{
	v2 res;
	res.x = l.x+f;
	res.y = l.y+f;
	return res;
}

v2 operator -(const v2 l, const v2 r)
{
	v2 res;
	res.x = l.x-r.x;
	res.y = l.y-r.y;
	return res;
}

v2 operator -(const v2 l, float f)
{
	v2 res;
	res.x = l.x-f;
	res.y = l.y-f;
	return res;
}

v2 operator -(float f, const v2 l)
{
	v2 res;
	res.x = f- l.x;
	res.y = f- l.y;
	return res;
}

v2 operator *(const v2 v, const v2 r)
{
	v2 res;
	res.x = v.x*r.x;
	res.y = v.y*r.y;
	return res;
}


v2 operator *(const v2 v, float f)
{
	v2 res;
	res.x = v.x*f;
	res.y = v.y*f;
	return res;
}

v2 operator *(float f, const v2 v)
{
	v2 res;
	res.x = v.x*f;
	res.y = v.y*f;
	return res;
}


v2 operator /(const v2 v, const v2 r)
{
	v2 res;
	res.x = v.x/r.x;
	res.y = v.y/r.y;
	return res;
}


v2 operator /(const v2 v, float f)
{
	v2 res;
	res.x = v.x/f;
	res.y = v.y/f;
	return res;
}

v2 operator /(float f, const v2 v)
{
	v2 res;
	res.x = f/v.x;
	res.y = f/v.y;
	return res;
}

float v2length(v2 v)
{
	return sqrt(v.x * v.x + v.y * v.y);
}

float v2dot(v2 v)
{
	return v.x * v.x + v.y * v.y;
}

float v2dot(v2 v0, v2 v1)
{
	return v0.x * v1.x + v0.y * v1.y;
}

v2 v2normalize(v2 v)
{
	if(v2iszero(v)) return v2init(0.0f,0.0f);
	return v / v2length(v);
}

v2 v2hat(v2 v)
{
	v2 r;
	r.x = v.y;
	r.y = -v.x;
	return r;
}

v2 v2reflect(v2 normal, v2 direction)
{
	return direction - normal * 2.f* v2dot(direction, normal);
}

float sign(float foo)
{
	return foo > 0 ? 1.f : -1.f;
}
v2 v2sign(v2 v)
{
	v2 r;
	r.x = sign(v.x);
	r.y = sign(v.y);
	return r;
}
v2 v2abs(v2 v)
{
	v2 r;
	r.x = fabs(v.x);
	r.y = fabs(v.y);
	return r;
}
v2 v2round(v2 v)
{
	return v2init(round(v.x), round(v.y));
}
float v2findpenetration(v2 pos, v2 dir, v2 size, v2 p0, v2 p1, v2 p2, v2 p3, v2& dirout, v2& pointout)
{
	ZASSERT(abs(v2length(dir)-1.f) < 1e-4f);
	v2 dirhat = v2hat(dir);
	p0 = p0 - pos;
	p1 = p1 - pos;
	p2 = p2 - pos;
	p3 = p3 - pos;
	float fPenetration = 0;
	float fSign = 1;
	v2 dirout_ = v2zero();
	v2 pointout_ = v2zero();

	float dp0 = v2dot(p0,dir);
	float dp1 = v2dot(p1,dir);
	float dp2 = v2dot(p2,dir);
	float dp3 = v2dot(p3,dir);
	float dp0hat = v2dot(p0,dirhat);
	float dp1hat = v2dot(p1,dirhat);
	float dp2hat = v2dot(p2,dirhat);
	float dp3hat = v2dot(p3,dirhat);
	float d0 = fabs(dp0);
	float d1 = fabs(dp1);
	float d2 = fabs(dp2);
	float d3 = fabs(dp3);
	float d0hat = fabs(dp0hat);
	float d1hat = fabs(dp1hat);
	float d2hat = fabs(dp2hat);
	float d3hat = fabs(dp3hat);

 	if(d0 < size.x && d0hat < size.y)
 	{
 		float fDist = size.x-d0;
 		float fDistHat = size.y-d0hat;
 		float fMinDist = Min(fDist, fDistHat);
 		if(fMinDist > fPenetration)
 		{
 			pointout_ = p0;
 			fPenetration = fDist;
 			if(fDist < fDistHat)
 				dirout_ = dir * sign(dp0)*(size.x - d0);
 			else
 				dirout_ = dirhat * sign(dp0hat) * (size.y - d0hat);
 		}
	}
 	if(d1 < size.x && d1hat < size.y)
 	{
 		float fDist = size.x-d1;
 		float fDistHat = size.y-d1hat;
 		float fMinDist = Min(fDist, fDistHat);
 		if(fMinDist > fPenetration)
 		{
 			pointout_ = p1;
 			fPenetration = fDist;
 			if(fDist < fDistHat)
 				dirout_ = dir * sign(dp1)*(size.x - d1);
 			else
 				dirout_ = dirhat * sign(dp1hat) * (size.y - d1hat);
 		}
 	}
 	if(d2 < size.x && d2hat < size.y)
 	{
 		float fDist = size.x-d2;
 		float fDistHat = size.y-d2hat;
  		float fMinDist = Min(fDist, fDistHat);
 		if(fMinDist > fPenetration)
 		{
 			pointout_ = p2;
 			fPenetration = fDist;
 			if(fDist < fDistHat)
 				dirout_ = dir * sign(dp2)*(size.x - d2);
 			else
 				dirout_ = dirhat * sign(dp2hat) * (size.y - d2hat);
 		}
 	}
 	if(d3 < size.x && d3hat < size.y)
 	{
 		float fDist = size.x-d3;
 		float fDistHat = size.y-d3hat;
  		float fMinDist = Min(fDist, fDistHat);
 		if(fMinDist > fPenetration)
 		{
 			pointout_ = p3;
 			fPenetration = fDist;
 			if(fDist < fDistHat)
 				dirout_ = dir * sign(dp3)*(size.x - d3);
 			else
 				dirout_ = dirhat * sign(dp3hat) * (size.y - d3hat);
 		}
 	}
 	dirout = dirout_;
 	pointout = pointout_ + pos;
 	return fPenetration;
}


void v2createcorners(v2 vPosition, v2 vDirection, v2 vSize, v2& p0, v2& p1, v2& p2, v2& p3)
{
	v2 vDirScale = vDirection * vSize.x;
	v2 vDirScaleHat = v2hat(vDirection) * vSize.y;
	p0 = vPosition + vDirScale + vDirScaleHat;
	p1 = vPosition - vDirScale + vDirScaleHat;
	p2 = vPosition - vDirScale - vDirScaleHat;
	p3 = vPosition + vDirScale - vDirScaleHat;
}

void v2createbounds(v2 vPosition, v2 vDirection, v2 vSize, v2& min_, v2& max_)
{
	v2 p0,p1,p2,p3;
	v2createcorners(vPosition, vDirection, vSize, p0, p1, p2, p3);
	min_ = v2min(v2min(p2,p3), v2min(p0,p1));
	max_ = v2max(v2max(p2,p3), v2max(p0,p1));
}


#define FLOAT_IS_OK(f) (f <= FLT_MAX && f >= -FLT_MAX)

bool v2fucked(v2 v)
{
	return (!(FLOAT_IS_OK(v.x) && FLOAT_IS_OK(v.y)));
}


v2 v2fromangle(float angle)
{
	v2 r;
	r.x = cosf(angle);
	r.y = sinf(angle);
	return r;
}	

void v3::operator +=(const v3& r)
{
	x += r.x;
	y += r.y;
	z += r.z;
}
void v3::operator -=(const v3& r)
{
	x -= r.x;
	y -= r.y;
	z -= r.z;
}
void v3::operator *=(const v3& r)
{
	x *= r.x;
	y *= r.y;
	z *= r.z;
}
void v3::operator /=(const v3& r)
{
	x /= r.x;
	y /= r.y;
	z /= r.z;
}

void v3::operator +=(const float r)
{
	x += r;
	y += r;
	z += r;
}
void v3::operator -=(const float r)
{
	x -= r;
	y -= r;
	z -= r;
}
void v3::operator *=(const float r)
{
	x *= r;
	y *= r;
	z *= r;
}
void v3::operator /=(const float r)
{
	x /= r;
	y /= r;
	z /= r;
}

v3 operator +(const v3 l, const v3 r)
{
	v3 res;
	res.x = l.x+r.x;
	res.y = l.y+r.y;
	res.z = l.z+r.z;	
	return res;
}

v3 operator +(const v3 l, float f)
{
	v3 res;
	res.x = l.x+f;
	res.y = l.y+f;
	res.z = l.z+f;	
	return res;
}

v3 operator +(float f, const v3 l)
{
	v3 res;
	res.x = l.x+f;
	res.y = l.y+f;
	res.z = l.z+f;	
	return res;
}

v3 operator -(const v3 l, const v3 r)
{
	v3 res;
	res.x = l.x-r.x;
	res.y = l.y-r.y;
	res.z = l.z-r.z;	
	return res;
}

v3 operator -(const v3 l, float f)
{
	v3 res;
	res.x = l.x-f;
	res.y = l.y-f;
	res.z = l.z-f;	
	return res;
}

v3 operator -(float f, const v3 l)
{
	v3 res;
	res.x = f - l.x;
	res.y = f - l.y;
	res.z = f - l.z;	
	return res;
}

v3 operator *(const v3 v, const v3 r)
{
	v3 res;
	res.x = v.x*r.x;
	res.y = v.y*r.y;
	res.z = v.z*r.z;	
	return res;
}


v3 operator *(const v3 v, float f)
{
	v3 res;
	res.x = v.x*f;
	res.y = v.y*f;
	res.z = v.z*f;	
	return res;
}

v3 operator *(float f, const v3 v)
{
	v3 res;
	res.x = v.x*f;
	res.y = v.y*f;
	res.z = v.z*f;	
	return res;
}


v3 operator /(const v3 v, const v3 r)
{
	v3 res;
	res.x = v.x/r.x;
	res.y = v.y/r.y;
	res.z = v.z/r.z;	
	return res;
}


v3 operator /(const v3 v, float f)
{
	v3 res;
	res.x = v.x/f;
	res.y = v.y/f;
	res.z = v.z/f;	
	return res;
}

v3 operator /(float f, const v3 v)
{
	v3 res;
	res.x = f / v.x;
	res.y = f / v.y;
	res.z = f / v.z;	
	return res;
}


v3 operator -(const v3 v)
{
	v3 r;
	r.x = -v.x;
	r.y = -v.y;
	r.z = -v.z;
	return r;
}

v2 v3::tov2()
{
	v2 r;
	r.x = x;
	r.y = y;
	return r;
}
v4 v3::tov4()
{
	v4 r;
	r.x = x;
	r.y = y;
	r.z = z;
	r.w = 0.f;
	return r;
}

v4 v3::tov4(float w)
{
	v4 r;
	r.x = x;
	r.y = y;
	r.z = z;
	r.w = w;
	return r;
}


v3 v3cross(v3 v0, v3 v1)
{
	v3 r;
	r.x = v0.y * v1.z - v0.z * v1.y;
	r.y = v0.z * v1.x - v0.x * v1.z;
	r.z = v0.x * v1.y - v0.y * v1.x;
	return r;
}
float v3lengthsq(v3 v)
{	
	return v3dot(v, v);
}

float v3length(v3 v)
{	
	return sqrt(v3dot(v,v));
}
v3 v3normalize(v3 v)
{
	return v / v3length(v);

}
v3 v3min(v3 a, v3 b)
{
	v3 r;
	r.x = a.x < b.x ? a.x : b.x;
	r.y = a.y < b.y ? a.y : b.y;
	r.z = a.z < b.z ? a.z : b.z;
	return r;
}

v3 v3max(v3 a, v3 b)
{
	v3 r;
	r.x = a.x >= b.x ? a.x : b.x;
	r.y = a.y >= b.y ? a.y : b.y;
	r.z = a.z >= b.z ? a.z : b.z;
	return r;
}

v3 v3abs(v3 a)
{
	v3 r;
	r.x = fabs(a.x);
	r.y = fabs(a.y);
	r.z = fabs(a.z);
	return r;
}
v3 v3splatx(v3 v)
{
	v3 r;
	r.x = v.x;
	r.y = v.x;
	r.z = v.x;
	return r;
}
v3 v3splaty(v3 v)
{
	v3 r;
	r.x = v.y;
	r.y = v.y;
	r.z = v.y;
	return r;
}
v3 v3splatz(v3 v)
{
	v3 r;
	r.x = v.z;
	r.y = v.z;
	r.z = v.z;
	return r;
}
v3 v4::tov3()
{
	v3 r;
	r.x = x;
	r.y = y;
	r.z = z;
	return r;
}

v2 v4::tov2()
{
	v2 r;
	r.x = x;
	r.y = y;
	return r;
}



float v4dot(v4 v0, v4 v1)
{
	return v0.x * v1.x + v0.y * v1.y + v0.z * v1.z + v0.w * v1.w;
}


void v4::operator +=(const v4& r)
{
	x += r.x;
	y += r.y;
	z += r.z;
	w += r.w;
}
void v4::operator -=(const v4& r)
{
	x -= r.x;
	y -= r.y;
	z -= r.z;
	w -= r.w;
}
void v4::operator *=(const v4& r)
{
	x *= r.x;
	y *= r.y;
	z *= r.z;
	w *= r.w;
}
void v4::operator /=(const v4& r)
{
	x /= r.x;
	y /= r.y;
	z /= r.z;
	w /= r.w;
}

void v4::operator +=(const float r)
{
	x += r;
	y += r;
	z += r;
	w += r;
}
void v4::operator -=(const float r)
{
	x -= r;
	y -= r;
	z -= r;
	w -= r;
}
void v4::operator *=(const float r)
{
	x *= r;
	y *= r;
	z *= r;
	w *= r;
}
void v4::operator /=(const float r)
{
	x /= r;
	y /= r;
	z /= r;
	w /= r;
}

v4 operator +(const v4 l, const v4 r)
{
	v4 res;
	res.x = l.x+r.x;
	res.y = l.y+r.y;
	res.z = l.z+r.z;	
	res.w = l.w+r.w;	
	return res;
}

v4 operator +(const v4 l, float f)
{
	v4 res;
	res.x = l.x+f;
	res.y = l.y+f;
	res.z = l.z+f;	
	res.w = l.w+f;	
	return res;
}

v4 operator +(float f, const v4 l)
{
	v4 res;
	res.x = l.x+f;
	res.y = l.y+f;
	res.z = l.z+f;	
	res.w = l.w+f;	
	return res;
}

v4 operator -(const v4 l, const v4 r)
{
	v4 res;
	res.x = l.x-r.x;
	res.y = l.y-r.y;
	res.z = l.z-r.z;	
	res.w = l.w-r.w;	
	return res;
}

v4 operator -(const v4 l, float f)
{
	v4 res;
	res.x = l.x-f;
	res.y = l.y-f;
	res.z = l.z-f;	
	res.w = l.w-f;	
	return res;
}

v4 operator -(float f, const v4 l)
{
	v4 res;
	res.x = f - l.x;
	res.y = f - l.y;
	res.z = f - l.z;	
	res.w = f - l.w;	
	return res;
}

v4 operator *(const v4 v, const v4 r)
{
	v4 res;
	res.x = v.x*r.x;
	res.y = v.y*r.y;
	res.z = v.z*r.z;	
	res.w = v.w*r.w;
	return res;
}


v4 operator *(const v4 v, float f)
{
	v4 res;
	res.x = v.x*f;
	res.y = v.y*f;
	res.z = v.z*f;	
	res.w = v.w*f;	
	return res;
}

v4 operator *(float f, const v4 v)
{
	v4 res;
	res.x = v.x*f;
	res.y = v.y*f;
	res.z = v.z*f;	
	res.w = v.w*f;	
	return res;
}


v4 operator /(const v4 v, const v4 r)
{
	v4 res;
	res.x = v.x/r.x;
	res.y = v.y/r.y;
	res.z = v.z/r.z;	
	res.w = v.w/r.w;
	return res;
}


v4 operator /(const v4 v, float f)
{
	v4 res;
	res.x = v.x/f;
	res.y = v.y/f;
	res.z = v.z/f;	
	res.w = v.w/f;
	return res;
}

v4 operator /(float f, const v4 v)
{
	v4 res;
	res.x = f / v.x;
	res.y = f / v.y;
	res.z = f / v.z;	
	res.w = f / v.w;
	return res;
}


v4 operator -(const v4 v)
{
	v4 r;
	r.x = -v.x;
	r.y = -v.y;
	r.z = -v.z;
	r.w = -v.w;
	return r;
}




m minit(v3 vx, v3 vy, v3 vz, v3 vtrans)
{
	m r;
	r.x = vx.tov4();
	r.y = vy.tov4();
	r.z = vz.tov4();
	r.trans = v4init(vtrans, 1);
	return r;
}
m minit(v4 vx, v4 vy, v4 vz, v4 vtrans)
{
	m r;
	r.x = vx;
	r.y = vy;
	r.z = vz;
	r.trans = vtrans;
	return r;
}

m mid()
{
	m r;
	r.x = v4init(1,0,0,0);
	r.y = v4init(0,1,0,0);
	r.z = v4init(0,0,1,0);
	r.trans = v4init(0,0,0,1);
	return r;
}
m mcreate(v3 vDir, v3 vRight, v3 vPoint)
{
	v3 vUp = v3normalize(v3cross(vRight, vDir));
	m r0 = mid();
	msetxaxis(r0, vRight);
	msetyaxis(r0, vUp);
	msetzaxis(r0, -vDir);
	m rt = mtranslate(-vPoint);
	m mtotales = mmult(r0, rt);
	v3 v0 = v3init(0,0,0);
	v3 v1 = mtransform(rt, v0);
	v3 v2 = mtransform(r0, v1);
	v3 v3__ = mtransform(mtotales, v0);

	v3 vrotdir = mtransform(r0, vDir);
	v3 vrotright = mtransform(r0, vRight);
	v3 vrotupr = mtransform(r0, vUp);

	ZASSERTAFFINE(rt);
	ZASSERTAFFINE(r0);
	ZASSERTAFFINE(mtotales);


	return mtotales;
}
m mmult(m m0, m m1)
{
	m r;
	r.x.x = m0.x.x * m1.x.x + m0.y.x * m1.x.y + m0.z.x * m1.x.z + m0.w.x * m1.x.w;
	r.x.y = m0.x.y * m1.x.x + m0.y.y * m1.x.y + m0.z.y * m1.x.z + m0.w.y * m1.x.w;
	r.x.z = m0.x.z * m1.x.x + m0.y.z * m1.x.y + m0.z.z * m1.x.z + m0.w.z * m1.x.w;
	r.x.w = m0.x.w * m1.x.x + m0.y.w * m1.x.y + m0.z.w * m1.x.z + m0.w.w * m1.x.w;

	r.y.x = m0.x.x * m1.y.x + m0.y.x * m1.y.y + m0.z.x * m1.y.z + m0.w.x * m1.y.w;
	r.y.y = m0.x.y * m1.y.x + m0.y.y * m1.y.y + m0.z.y * m1.y.z + m0.w.y * m1.y.w;
	r.y.z = m0.x.z * m1.y.x + m0.y.z * m1.y.y + m0.z.z * m1.y.z + m0.w.z * m1.y.w;
	r.y.w = m0.x.w * m1.y.x + m0.y.w * m1.y.y + m0.z.w * m1.y.z + m0.w.w * m1.y.w;

	r.z.x = m0.x.x * m1.z.x + m0.y.x * m1.z.y + m0.z.x * m1.z.z + m0.w.x * m1.z.w;
	r.z.y = m0.x.y * m1.z.x + m0.y.y * m1.z.y + m0.z.y * m1.z.z + m0.w.y * m1.z.w;
	r.z.z = m0.x.z * m1.z.x + m0.y.z * m1.z.y + m0.z.z * m1.z.z + m0.w.z * m1.z.w;
	r.z.w = m0.x.w * m1.z.x + m0.y.w * m1.z.y + m0.z.w * m1.z.z + m0.w.w * m1.z.w;


	r.w.x = m0.x.x * m1.w.x + m0.y.x * m1.w.y + m0.z.x * m1.w.z + m0.w.x * m1.w.w;
	r.w.y = m0.x.y * m1.w.x + m0.y.y * m1.w.y + m0.z.y * m1.w.z + m0.w.y * m1.w.w;
	r.w.z = m0.x.z * m1.w.x + m0.y.z * m1.w.y + m0.z.z * m1.w.z + m0.w.z * m1.w.w;
	r.w.w = m0.x.w * m1.w.x + m0.y.w * m1.w.y + m0.z.w * m1.w.z + m0.w.w * m1.w.w;


	// r.x.x = m0.x.x * m1.x.x + m0.x.y * m1.y.x + m0.x.z * m1.z.x + m0.x.w * m1.w.x; 
	// r.x.y = m0.x.x * m1.x.y + m0.x.y * m1.y.y + m0.x.z * m1.z.y + m0.x.w * m1.w.y; 
	// r.x.z = m0.x.x * m1.x.z + m0.x.y * m1.y.z + m0.x.z * m1.z.z + m0.x.w * m1.w.z; 
	// r.x.w = m0.x.x * m1.x.w + m0.x.y * m1.y.w + m0.x.z * m1.z.w + m0.x.w * m1.w.w; 

	// r.y.x = m0.y.x * m1.x.x + m0.y.y * m1.y.x + m0.y.z * m1.z.x + m0.y.w * m1.w.x; 
	// r.y.y = m0.y.x * m1.x.y + m0.y.y * m1.y.y + m0.y.z * m1.z.y + m0.y.w * m1.w.y; 
	// r.y.z = m0.y.x * m1.x.z + m0.y.y * m1.y.z + m0.y.z * m1.z.z + m0.y.w * m1.w.z; 
	// r.y.w = m0.y.x * m1.x.w + m0.y.y * m1.y.w + m0.y.z * m1.z.w + m0.y.w * m1.w.w; 

	// r.z.x = m0.z.x * m1.x.x + m0.z.y * m1.y.x + m0.z.z * m1.z.x + m0.z.w * m1.w.x; 
	// r.z.y = m0.z.x * m1.x.y + m0.z.y * m1.y.y + m0.z.z * m1.z.y + m0.z.w * m1.w.y; 
	// r.z.z = m0.z.x * m1.x.z + m0.z.y * m1.y.z + m0.z.z * m1.z.z + m0.z.w * m1.w.z; 
	// r.z.w = m0.z.x * m1.x.w + m0.z.y * m1.y.w + m0.z.z * m1.z.w + m0.z.w * m1.w.w; 

	// r.w.x = m0.w.x * m1.x.x + m0.w.y * m1.y.x + m0.w.z * m1.z.x + m0.w.w * m1.w.x; 
	// r.w.y = m0.w.x * m1.x.y + m0.w.y * m1.y.y + m0.w.z * m1.z.y + m0.w.w * m1.w.y; 
	// r.w.z = m0.w.x * m1.x.z + m0.w.y * m1.y.z + m0.w.z * m1.z.z + m0.w.w * m1.w.z; 
	// r.w.w = m0.w.x * m1.x.w + m0.w.y * m1.y.w + m0.w.z * m1.z.w + m0.w.w * m1.w.w; 

	return r;
}
m mrotatex(float fAngle)
{
	m r = mid();
	float ca = cos(fAngle);
	float sa = sin(fAngle);
	r.y.y = ca;
	r.y.z = sa;
	r.z.y = -sa;
	r.z.z = ca;
	ZASSERTAFFINE(r);
	return r;
}
m mrotatey(float fAngle)
{
	m r = mid();
	float ca = cos(fAngle);
	float sa = sin(fAngle);
	r.x.x = ca;
	r.x.z = -sa;
	r.z.x = sa;
	r.z.z = ca;
	ZASSERTAFFINE(r);

	return r;	
}
m mrotatez(float fAngle)
{
	m r = mid();
	float ca = cos(fAngle);
	float sa = sin(fAngle);
	r.x.x = ca;
	r.x.y = sa;
	r.y.x = -sa;
	r.y.y = ca;
	ZASSERTAFFINE(r);

	return r;	

}
m mtranslate(v3 trans)
{
	m r = mid();
	r.trans = v4init(trans,1.f);
	return r;
}

v3 mtransform(m mat, v3 point)
{
	v3 r;
	r.x = mat.x.x * point.x + mat.y.x * point.y + mat.z.x * point.z;
	r.y = mat.x.y * point.x + mat.y.y * point.y + mat.z.y * point.z;
	r.z = mat.x.z * point.x + mat.y.z * point.y + mat.z.z * point.z;
	r += v3init(mat.trans);
	return r;
}

v3 mrotate(m mat, v3 point)
{
	v3 r;
	r.x = mat.x.x * point.x + mat.y.x * point.y + mat.z.x * point.z;
	r.y = mat.x.y * point.x + mat.y.y * point.y + mat.z.y * point.z;
	r.z = mat.x.z * point.x + mat.y.z * point.y + mat.z.z * point.z;
	return r;
}

m mperspective(float fFovY, float fAspect, float fNear, float fFar)\
{
	//f aspect 0 0 0  -- 
	// 0 f 0 0 -- 
	// 0 0 zFar + zNear zNear - zFar 2 × zFar × zNear zNear - zFar 
	// 0 0 -1 0

	float ymax = fNear * tan(fFovY * PI / 360.f);
	float ymin = -ymax;
	float xmax = ymax * fAspect;
	float xmin = ymin * fAspect;

	float width = ymax - xmin;
	float height = xmax - ymin;
	float w = 2 * fNear / width;
	w = w / fAspect;
	float h = 2 * fNear / height;


	float fAngle = fFovY * PI / 360.f;
	float f = cos(fAngle)/sin(fAngle);

	float fDepth = fFar - fNear;
	float q = -(fFar + fNear) / fDepth;
	float qn =  -2 * (fFar + fNear) / fDepth;
	m r = mid();
	r.x.x = w;
	r.y.y = h;
	r.z.z = q;
	r.z.w = -1.f;
	r.w.z = qn;
	return r;
}


m mortho(float fX, float fY, float fZRange)
{
	m r = mid();
	r.x.x = 2.f / fX;
	r.y.y = 2.f / fY;
	r.z.z = 1.f / fZRange;
	return r;

}

void msetxaxis(m& mat, v3 axis)
{
	mat.x.x = axis.x;
	mat.y.x = axis.y;
	mat.z.x = axis.z;
}
void msetyaxis(m& mat, v3 axis)
{
	mat.x.y = axis.x;
	mat.y.y = axis.y;
	mat.z.y = axis.z;
}
void msetzaxis(m& mat, v3 axis)
{
	mat.x.z = axis.x;
	mat.y.z = axis.y;
	mat.z.z = axis.z;
}
m minverserotation(m mat)
{
	m r = mat;

	r.x.z = mat.z.x;
	r.z.x = mat.x.z;

	r.x.y = mat.y.x;
	r.y.x = mat.x.y;

	r.y.z = mat.z.y;
	r.z.y = mat.y.z;
	return r;

}

void ZASSERTAFFINE(m mat)
{
	ZASSERT(fabs(1-v3length(mat.x.tov3())) < 0.001f);
	ZASSERT(fabs(1-v3length(mat.y.tov3())) < 0.001f);
	ZASSERT(fabs(1-v3length(mat.z.tov3())) < 0.001f);


	float d0 = v3dot(mat.x.tov3(), mat.y.tov3());
	float d1 = v3dot(mat.x.tov3(), mat.z.tov3());
	float d2 = v3dot(mat.z.tov3(), mat.y.tov3());
	ZASSERT(fabs(d0) < 0.001f);
	ZASSERT(fabs(d1) < 0.001f);
	ZASSERT(fabs(d2) < 0.001f);

}


v3 obbtoaabb(m mrotation, v3 vHalfSize)
{
	v3 vx = v3abs(mrotation.x.tov3() * vHalfSize.x);
	v3 vy = v3abs(mrotation.y.tov3() * vHalfSize.y);
	v3 vz = v3abs(mrotation.z.tov3() * vHalfSize.z);
	return vx + vy + vz;
}



v2 v2randir()
{
	v2 v;
	do
	{
		v.x = randrange(-1000,1000);
		v.y = randrange(-1000,1000);
	}while(v2length2(v) < 1e-3f);
	return v2normalize(v);
}

v2 v2randdisc()
{
	return v2init(frand()*2.0f-1.0f, frand()*2.0f-1.0f);
}

int32_t randrange(int32_t nmin, int32_t nmax)
{
	if(nmin == nmax) return nmin;
	int r = rand();
	return nmin + (r % (nmax-nmin));
}

float frand()
{
	return (float)rand() / RAND_MAX;
}

float frandrange(float fmin, float fmax)
{
	return frand() * (fmax-fmin) + fmin;	
}

uint64_t rand64()
{
	//todo add better rand
	return rand();
}
uint64_t rand64(uint64_t)
{
	//todo: borken
	return rand();
}
