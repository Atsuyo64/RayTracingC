#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "moremath.h"
#include "scene.h"

float length(vec3 v)
{
   return sqrt(v.x*v.x+v.y*v.y+v.z*v.z);
}

vec3 normalized(vec3 v)
{
   float invLen = 1./length(v);
   vec3 res = {v.x*invLen,v.y*invLen,v.z*invLen};
   return res;
}

uint8 floatToUint(float f)
{
   if(f<0)
      return 0;
   return f>=1?255:(uint8)(f*255.f);
}


float dot(vec3 a,vec3 b)
{
   return a.x*b.x+a.y*b.y+a.z*b.z;
}

float clamp(float x)
{
   return x<0?0:(x>1?1:x);
}

vec3 cross(vec3 u,vec3 v)//right hand cross product
{
   vec3 res = {u.y*v.z-u.z*v.y,u.z*v.x-u.x*v.z,u.x*v.y-u.y*v.x};
   return res;
}

float smoothstep(float inf,float sup,float x)
{
   x = clamp((x-inf)/(sup-inf));
   return x*x*(3.-2.*x);
}

vec3 plus(vec3 a,vec3 b)
{
   vec3 res={a.x+b.x,a.y+b.y,a.z+b.z};
   return res;
}

vec3 minus(vec3 a,vec3 b)
{
   vec3 res={a.x-b.x,a.y-b.y,a.z-b.z};
   return res;
}

vec3 times(vec3 a,float b)
{
   vec3 res={a.x*b,a.y*b,a.z*b};
   return res;
}

vec3 timesVec3(vec3 a,vec3 b)
{
   vec3 res={a.x*b.x,a.y*b.y,a.z*b.z};
   return res;
}

vec3 reflect(vec3 dir,vec3 normal)
{
   return minus(dir,times(normal,2.*dot(dir,normal)));//dir - 2. * dot(dir,normal) * normal;
}

vec3 lerp(vec3 inf,vec3 sup,float t)
{
   return plus(times(inf,1-t),times(sup,t));
}

float RandomValue()//U(0,1)
{
   rngState = rngState * 747796405 + 2891336453;
   unsigned int result = ((rngState >> ((rngState >> 28) + 4)) ^ rngState) * 277803737;
   result = (result >> 22) ^ result;
   return result/4294967295.0;
}

float RandomValueNormalDistrubtion()//G(0,1)
{
   float theta = 2 * 3.14159265 * RandomValue();
   float rho = sqrt(-2*log(RandomValue()));
   return rho * cos(theta);
}

vec3 RandomDiretion()
{
   vec3 res = {RandomValueNormalDistrubtion(),RandomValueNormalDistrubtion(),RandomValueNormalDistrubtion()};
   return normalized(res);
}

vec3 RandomHemisphereDirection(vec3 normal)
{
   vec3 dir = RandomDiretion();
   return dot(dir,normal)<0?times(dir,-1):dir;
}
