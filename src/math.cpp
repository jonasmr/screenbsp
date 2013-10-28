#include <math.h>
#include <float.h>
#include <stdlib.h>

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
uint32_t v3::tocolor()
{
	return 0xff000000 
		| ((uint32_t(z * 255.f) << 16)&0xff0000)
		| ((uint32_t(y * 255.f) << 8)&0xff00)
		| (uint32_t(z * 255.f));
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

float v3distance(v3 p0, v3 p1)
{
	return v3length(p0 - p1);
}

v3 v3fromcolor(uint32_t nColor)
{
	float fMul = 1.f / 255.f;
	v3 r;
	r.x = (nColor&0xff) * fMul;
	r.y = ((nColor>>8)&0xff) * fMul;
	r.z = ((nColor>>16)&0xff) * fMul;
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
v3 v3min(v3 a, float f)
{
	v3 r;
	r.x = a.x < f ? a.x : f;
	r.y = a.y < f ? a.y : f;
	r.z = a.z < f ? a.z : f;
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
v3 v3max(v3 a, float f)
{
	v3 r;
	r.x = a.x >= f ? a.x : f;
	r.y = a.y >= f ? a.y : f;
	r.z = a.z >= f ? a.z : f;
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
uint32_t v4::tocolor()
{
	return ((uint32_t(w * 255.f) << 24)&0xff000000)
		| ((uint32_t(z * 255.f) << 16)&0xff0000)
		| ((uint32_t(y * 255.f) << 8)&0xff00)
		| (uint32_t(z * 255.f));
}

v4 v4fromcolor(uint32_t nColor)
{
	float fMul = 1.f / 255.f;
	v4 r;
	r.x = (nColor&0xff) * fMul;
	r.y = ((nColor>>8)&0xff) * fMul;
	r.z = ((nColor>>16)&0xff) * fMul;
	r.w = ((nColor>>24)&0xff) * fMul;
	return r;
}


v4 v4neg(v4 v)
{
	v.x = -v.x;
	v.y = -v.y;
	v.z = -v.z;
	v.w = -v.w;
	return v;
}

float v4dot(v4 v0, v4 v1)
{
	return v0.x * v1.x + v0.y * v1.y + v0.z * v1.z + v0.w * v1.w;
}

float v4length(v4 v0)
{
	return sqrt(v0.x * v0.x + v0.y * v0.y + v0.z * v0.z + v0.w * v0.w);
}

float v4length2(v4 v0)
{
	return v0.x * v0.x + v0.y * v0.y + v0.z * v0.z + v0.w * v0.w;
}


v4 v4makeplane(v3 p, v3 normal)
{
	float fDot = v3dot(normal,p);
	v4 r = v4init(normal, -(fDot));
	float vDot = v4dot(v4init(p, 1.f), r);
	if(fabsf(vDot) > 0.001f)
	{
		ZBREAK();
	}

	return r;
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

//cmajor check
m mmult(m m0, m m1)
{
	m r;
	r.x.x = m0.x.x * m1.x.x + m0.y.x * m1.x.y + m0.z.x * m1.x.z + m0.trans.x * m1.x.w;
	r.x.y = m0.x.y * m1.x.x + m0.y.y * m1.x.y + m0.z.y * m1.x.z + m0.trans.y * m1.x.w;
	r.x.z = m0.x.z * m1.x.x + m0.y.z * m1.x.y + m0.z.z * m1.x.z + m0.trans.z * m1.x.w;
	r.x.w = m0.x.w * m1.x.x + m0.y.w * m1.x.y + m0.z.w * m1.x.z + m0.trans.w * m1.x.w;

	r.y.x = m0.x.x * m1.y.x + m0.y.x * m1.y.y + m0.z.x * m1.y.z + m0.trans.x * m1.y.w;
	r.y.y = m0.x.y * m1.y.x + m0.y.y * m1.y.y + m0.z.y * m1.y.z + m0.trans.y * m1.y.w;
	r.y.z = m0.x.z * m1.y.x + m0.y.z * m1.y.y + m0.z.z * m1.y.z + m0.trans.z * m1.y.w;
	r.y.w = m0.x.w * m1.y.x + m0.y.w * m1.y.y + m0.z.w * m1.y.z + m0.trans.w * m1.y.w;

	r.z.x = m0.x.x * m1.z.x + m0.y.x * m1.z.y + m0.z.x * m1.z.z + m0.trans.x * m1.z.w;
	r.z.y = m0.x.y * m1.z.x + m0.y.y * m1.z.y + m0.z.y * m1.z.z + m0.trans.y * m1.z.w;
	r.z.z = m0.x.z * m1.z.x + m0.y.z * m1.z.y + m0.z.z * m1.z.z + m0.trans.z * m1.z.w;
	r.z.w = m0.x.w * m1.z.x + m0.y.w * m1.z.y + m0.z.w * m1.z.z + m0.trans.w * m1.z.w;


	r.trans.x = m0.x.x * m1.trans.x + m0.y.x * m1.trans.y + m0.z.x * m1.trans.z + m0.trans.x * m1.trans.w;
	r.trans.y = m0.x.y * m1.trans.x + m0.y.y * m1.trans.y + m0.z.y * m1.trans.z + m0.trans.y * m1.trans.w;
	r.trans.z = m0.x.z * m1.trans.x + m0.y.z * m1.trans.y + m0.z.z * m1.trans.z + m0.trans.z * m1.trans.w;
	r.trans.w = m0.x.w * m1.trans.x + m0.y.w * m1.trans.y + m0.z.w * m1.trans.z + m0.trans.w * m1.trans.w;


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

m mscale(float fScale)
{
	m r = mid();
	r.x.x = r.y.y = r.z.z = fScale;
	return r;
}
m mscale(float fScaleX, float fScaleY, float fScaleZ)
{
	m r = mid();
	r.x.x = fScaleX;
	r.y.y = fScaleY;
	r.z.z = fScaleZ;
	return r;
}




m mtranslate(v3 trans)
{
	m r = mid();
	r.trans = v4init(trans,1.f);
	return r;
}

#define MAT_INDEX(i, j) (i*4+j)


//cmajor
v3 mtransform(m mat, v3 point)
{
	v3 r;
	r.x = mat.x.x * point.x + mat.y.x * point.y + mat.z.x * point.z;
	r.y = mat.x.y * point.x + mat.y.y * point.y + mat.z.y * point.z;
	r.z = mat.x.z * point.x + mat.y.z * point.y + mat.z.z * point.z;
	r += v3init(mat.trans);
	return r;
}


v4 mtransform(m mat, v4 v)
{
	v4 r;
	r.x = mat.x.x * v.x + mat.y.x * v.y + mat.z.x * v.z + mat.trans.x * v.w;
	r.y = mat.x.y * v.x + mat.y.y * v.y + mat.z.y * v.z + mat.trans.y * v.w;
	r.z = mat.x.z * v.x + mat.y.z * v.y + mat.z.z * v.z + mat.trans.z * v.w;
	r.w = mat.x.w * v.x + mat.y.w * v.y + mat.z.w * v.z + mat.trans.w * v.w;
	return r;
}

void mrowmul(int row, float scalar, float* fmat, float* finv)
{
	for(int i = 0;i < 4; ++i)
	{
		fmat[ MAT_INDEX(row, i) ] *= scalar;
		finv[ MAT_INDEX(row, i) ] *= scalar;
	}
}

void mrowmuladd(int rowsrc, int rowdst, float scalar, float* fmat, float* finv)
{
	for(int i = 0; i < 4; ++i)
	{
		fmat[ MAT_INDEX(rowdst, i) ] += scalar * fmat[MAT_INDEX(rowsrc,i)];
		finv[ MAT_INDEX(rowdst, i) ] += scalar * finv[MAT_INDEX(rowsrc,i)];
	}
}

void mreducedown(int i, float* fmat, float* finv)
{
	float* f = fmat;
	float valueSrc = f[ MAT_INDEX(i,i) ];
	if(fabs(valueSrc) < 1e-8)
		return;


	mrowmul( i, 1.f / f[MAT_INDEX(i,i)], f, finv);
	for(int j = i+1; j < 4; ++j)
	{
		float value = f[ MAT_INDEX(j,i) ];
		mrowmuladd(i, j, -value, f, finv);
		ZASSERT(fabs(f[ MAT_INDEX(j,i) ]) < 0.00001f);
	}
}
void mreduceup(int i, float* f, float* finv)
{
	float valueSrc = f[ MAT_INDEX(i,i) ];
	if(fabsf(valueSrc) < 1e-8)
		return;
	for(int j = i-1; j >= 0; --j)
	{
		float value = f[ MAT_INDEX(j,i) ];
		//			value - valueSrc * x == 0 // x == (value/valueSrc)
		mrowmuladd(i, j, -value, f, finv);
		ZASSERT(fabs(f[ MAT_INDEX(j,i) ]) < 0.00001f);
	}
}


m minverse(m mat)
{
	m inv = mid();
	float* fmat = (float*)&mat;
	float* finv = (float*)&inv;
	mreducedown(0, fmat, finv);
	mreducedown(1, fmat, finv);
	mreducedown(2, fmat, finv);
	mreducedown(3, fmat, finv);

	mreduceup(3, fmat, finv);
	mreduceup(2, fmat, finv);
	mreduceup(1, fmat, finv);
	return inv;
}

m mtranspose(m mat)
{
	m r;
	r.x.x = mat.x.x;
	r.x.y = mat.y.x;
	r.x.z = mat.z.x;
	r.x.w = mat.trans.x;

	r.y.x = mat.x.y;
	r.y.y = mat.y.y;
	r.y.z = mat.z.y;
	r.y.w = mat.trans.y;

	r.z.x = mat.x.z;
	r.z.y = mat.y.z;
	r.z.z = mat.z.z;
	r.z.w = mat.trans.z;

	r.trans.x = mat.x.w;
	r.trans.y = mat.y.w;
	r.trans.z = mat.z.w;
	r.trans.w = mat.trans.w;
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
m mviewport(float x, float y, float w, float h)
{
	m r = mid();
	r.trans.x = x + w * 0.5f;
	r.trans.y = y + h * 0.5f;
	r.x.x = w * 0.5f;
	r.y.y = h * 0.5f;
	return r;
}

m mperspective(float fFovY, float fAspect, float fNear, float fFar)
{
	m r = mid();
	float fAngle = (fFovY * PI / 180.f)/2.f;
	float f = cos(fAngle) / sin(fAngle);
	r.x.x = f * fAspect;
	r.y.y = f;
	r.z.z = (fFar + fNear) / (fNear - fFar);
	r.trans.z = (2*fFar*fNear) / (fNear - fFar);
	r.z.w = -1.f;
	r.trans.w = 0.f;
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

m morthogl(float left, float right, float bottom, float top, float near, float far)
{
	m mat = mid();
	mat.x.x = 2.0 / (right - left);
	mat.y.y = 2.0 / (top - bottom);
	mat.z.z = -2.0 / (far - near);
	mat.trans.x = - (right + left) / (right - left);
	mat.trans.y = - (top + bottom) / (top - bottom);
	mat.trans.z = - (far + near) / (far - near);

	return mat;
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

m maffineinverse(m mat)
{
	// p = trans + rot * x
	// p - trans = rot * x
	// rot^ * (p - trans) = x
	// rot^ * p + rot^ * (-trans) = x

	m mrot = minverserotation(mat);
	v3 trans = -mat.trans.tov3();
	mrot.trans = v4init(mrotate(mrot, trans), 1.f);
	return mrot;
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

float rayplaneintersect(v3 r0, v3 rdir, v4 plane)
{
	v3 vpoint = plane.tov3() * -plane.w;
	float fdot = v4dot(v4init(vpoint,1.f), plane);
	ZASSERT(fabs(fdot)<1e-5f);
	return rayplaneintersect(r0, rdir, plane.tov3() * -plane.w, plane.tov3());
}
float rayplaneintersect(v3 r0, v3 rdir, v3 p0, v3 pnormal)
{
	float t = -FLT_MAX;
	float dot0 = v3dot(rdir, pnormal);
	if(fabs(dot0)<1e-5f)
		return t;
	return v3dot((p0 - r0), pnormal);
}
float rayboxintersect(v3 r0, v3 rdir, m boxtransform, v3 boxsize)
{
	m inv = maffineinverse(boxtransform);
	rdir = mrotate(inv, rdir);
	r0 = mtransform(inv, r0);
	float tx0 = (boxsize.x - r0.x) / rdir.x;
	float tx1 = (-boxsize.x - r0.x) / rdir.x;
	float ty0 = (boxsize.y - r0.y) / rdir.y;
	float ty1 = (-boxsize.y - r0.y) / rdir.y;
	float tz0 = (boxsize.z - r0.z) / rdir.z;
	float tz1 = (-boxsize.z - r0.z) / rdir.z;
	float tnear = -FLT_MAX;
	float tfar = FLT_MAX;

	if(tx0 > tx1) 
		Swap(tx0, tx1);
	if(ty0 > ty1) 
		Swap(ty0, ty1);
	if(tz0 > tz1) 
		Swap(tz0, tz1);

	if(tnear < tx0)
		tnear = tx0;
	if(tfar > tx1)
		tfar = tx1;
	if(tnear < ty0)
		tnear = ty0;
	if(tfar > ty1)
		tfar = ty1;
	if(tnear < tz0)
		tnear = tz0;
	if(tfar > tz1)
		tfar = tz1;

	if(tnear > tfar)
		return INTERSECT_FAIL;
	if(tfar < 0)
		return INTERSECT_FAIL;
	return tnear;



	// float txmin = rayplaneintersect(r0, rdir, v3init(boxsize.x, 0, 0) * m.x + m.trans, m.x);
	// float txmax = rayplaneintersect(r0, rdir, v3init(-boxsize.x, 0, 0) * m.x + m.trans, m.x);
	// float tymin = rayplaneintersect(r0, rdir, v3init(0, boxsize.y, 0) * m.y + m.trans, m.y);
	// float tymax = rayplaneintersect(r0, rdir, v3init(0, -boxsize.y, 0) * m.y + m.trans, m.y);
	// float tzmin = rayplaneintersect(r0, rdir, v3init(0, 0, boxsize.z) * m.z + m.trans, m.z);
	// float tzmax = rayplaneintersect(r0, rdir, v3init(0, 0, -boxsize.z) * m.z + m.trans, m.z);
}

namespace
{
	uint32_t g_k = 0xed32babe;
	uint32_t g_j = 0xdeadf39c;
}

void randseed(uint32_t k, uint32_t j)
{
	g_k = k;
	g_j = j;
}
uint32_t rand32()
{
	g_k=30345*(g_k&65535)+(g_k>>16);
	g_j=18000*(g_j&65535)+(g_j>>16);
	return (((g_k << 16) | (g_k >> 16)) + g_j) % 0x7fff;
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
	int r = rand32();
	return nmin + (r % (nmax-nmin));
}

float frand()
{
	return (float)rand32() / 0x7fff;
}

float frandrange(float fmin, float fmax)
{
	return frand() * (fmax-fmin) + fmin;	
}

uint64_t rand64()
{
	//todo add better rand
	return rand32()| (((uint64)rand32())<<32);
}
uint64_t rand64(uint64_t)
{
	//todo: borken
	return rand32()| (((uint64)rand32())<<32);
}

v3 hsvtorgb(v3 hsv)
{
	float h = hsv.x;
	float s = hsv.y;
	float v = hsv.z;
	int hi = int(h*6);
	float f = h*6 - hi;
	float p = v * (1 - s);
	float q = v * (1 - f * s);
	float t = v * (1 - (1 - f) * s);
	switch(hi)
	{
	case 0: return v3init(v, t, p);
	case 1: return v3init(q, v, p);
	case 2: return v3init(p, v, t);
	case 3: return v3init(p, q, v);
	case 4: return v3init(t, p, v);
	default: return v3init(v, p, q);
	}

}

v3 v3randcolor()
{
	return hsvtorgb(v3init(frandrange(0.1,0.9), 0.5, 0.95f));
}

v3 v3randredcolor()
{
	return hsvtorgb(v3init(0, 0.5, frandrange(0.5, 0.95f)));
}
uint32 randcolor()
{
	v3 c = v3randcolor();
	uint32 r = 0;
	r |= (0xff&uint32(c.x * 255.f)) << 16;
	r |= (0xff&uint32(c.y * 255.f)) << 8;
	r |= (0xff&uint32(c.z * 255.f));
	return r;
}

uint32 randredcolor()
{
	v3 c = v3randredcolor();
	uint32 r = 0;
	r |= (0xff&uint32(c.x * 255.f)) << 16;
	r |= (0xff&uint32(c.y * 255.f)) << 8;
	r |= (0xff&uint32(c.z * 255.f));
	return r;
}

// # HSV values in [0..1[
// # returns [r, g, b] values from 0 to 255
// def hsv_to_rgb(h, s, v)
//   h_i = (h*6).to_i
//   f = h*6 - h_i
//   p = v * (1 - s)
//   q = v * (1 - f*s)
//   t = v * (1 - (1 - f) * s)
//   r, g, b = v, t, p if h_i==0
//   r, g, b = q, v, p if h_i==1
//   r, g, b = p, v, t if h_i==2
//   r, g, b = p, q, v if h_i==3
//   r, g, b = t, p, v if h_i==4
//   r, g, b = v, p, q if h_i==5
//   [(r*256).to_i, (g*256).to_i, (b*256).to_i]
// end
// # using HSV with variable hue
// gen_html { hsv_to_rgb(rand, 0.5, 0.95) }


