#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stbi_image_write.h"

typedef unsigned char uint8;  

typedef struct vec3
{
   float x,y,z;
} vec3;

typedef struct vec2
{
   float x,y;
} vec2;

typedef struct Color
{
   uint8 r,g,b;
} Color;

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

Color vec3ToColor(vec3 v)
{
   Color res = {floatToUint(v.x),floatToUint(v.y),floatToUint(v.z)};
   return res;
}

typedef struct Material
{
   vec3 color;
   float emissionStrength;
} Material;

typedef struct Sphere
{
   vec3 pos;
   float r;
   Material mat;
} Sphere;

typedef struct HitInfo
{
   int didHit;
   float dst;
   vec3 hitPoint;
   vec3 normal;
   Material mat;
} HitInfo;

typedef struct Ray
{
   vec3 pos;
   vec3 dir;
} Ray;
                                 //pos,radius,{color,emission}
static Sphere const spheres[] = {{{2,-.8,2},1,{{1,1,1},0}},
                                 {{.51,-.3,2},.5,{{1,0,0},0}},
                                 {{-1.5,.05,2},.4,{{0,1,0},0}},
                                 {{-2.2,.1,2},.3,{{.4,.4,1},0}},
                                 {{0,30.1,3},30,{{0.61,0,1},0}},
                                 //{{-15,-15,10},15,{{1,.85,.85},1}},
                                 };
                                 //{{{0,0.7071068,4},1,{{1,1,1},1}},{{0,0,2},1,{{1,0,0},1}}};//debug
static const int sphereCount = sizeof(spheres)/sizeof(Sphere);

static int const w = 128*4;//width
static int const h = 128*4;//height
static int const accumulationCount = 100*10;//1000 = ok
static int const maxBounce = 4+2;
static vec3 const sunDirection = {-30,-85,100};
static vec3 const SkyColorHorizon = {1,1,1};
static vec3 const SkyColorZenith = {0.263,0.969,0.871};//{.5,.5,1};
static vec3 const GroundColor = {.66,.66,.66};
static float const SunFocus = 22;
static float const SunIntensity = .75;


static vec3 normalizedSunDirection;
static int const halfW = w/2;
static int const halfH = h/2;
static unsigned int rngState;

float dot(vec3 a,vec3 b)
{
   return a.x*b.x+a.y*b.y+a.z*b.z;
}

float clamp(float x)
{
   return x<0?0:(x>1?1:x);
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

vec3 lerp(vec3 inf,vec3 sup,float t)
{
   return plus(times(inf,1-t),times(sup,t));
}

float RandomValue()
{
   rngState = rngState * 747796405 + 2891336453;
   unsigned int result = ((rngState >> ((rngState >> 28) + 4)) ^ rngState) * 277803737;
   result = (result >> 22) ^ result;
   return result/4294967295.0;
}

float RandomValueNormalDistrubtion()
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

vec3 getEnvironmentLight(Ray ray)
{
   float skyGradientT = powf(smoothstep(0,.74,-ray.dir.y),.35);
   vec3 skyGradient   = lerp(SkyColorHorizon,SkyColorZenith,skyGradientT);
   float sun = powf(fmax(0,dot(ray.dir,normalizedSunDirection)),SunFocus)*SunIntensity;
   float groundToSkyT = smoothstep(-0.01,0,-ray.dir.y);
   float sunMask = ray.dir.y<0;
   vec3 sunValue = {sun*sunMask,sun*sunMask,sun*sunMask};
   return plus(lerp(GroundColor,skyGradient,groundToSkyT),sunValue);
}

HitInfo raySphere(Ray ray,vec3 sphereCentre,float radius)
{
   vec3 offset = minus(ray.pos,sphereCentre);
   float b = dot(offset,ray.dir);
   float c = dot(offset,offset)-radius*radius;

   float delta = b*b-c;

   if(delta<0)
   {
      HitInfo res = {0};
      return res;
   }
   delta = sqrt(delta);
   float dst = -b-delta;
   if(dst<0)
      dst=-b+delta;
   if(dst<0)
   {
      HitInfo res = {0};
      return res;
   }
   vec3 hitPoint = plus(ray.pos,times(ray.dir,dst));
   HitInfo res = {1,dst,hitPoint,normalized(minus(hitPoint,sphereCentre))};
   return res;
}

HitInfo CalculateRayCollision(Ray ray)
{
   HitInfo closest = {0,999.f};
   for(int i=0;i<sphereCount;++i)
   {
      HitInfo hitInfo = raySphere(ray,spheres[i].pos,spheres[i].r);
      if(hitInfo.didHit && hitInfo.dst<closest.dst)
      {
         closest=hitInfo;
         closest.mat = spheres[i].mat;
      }
   }
   return closest;
}

vec3 calcColor(Ray ray,int maxBounce)
{
   //Color skyColor={0x77,0xB5,0xFE};
   vec3 incomingLight = {0,0,0};
   vec3 rayColor = {1,1,1};

   for(int i=0;i<maxBounce;++i)
   {
      HitInfo hitInfo = CalculateRayCollision(ray);
      //printf("[Bounce %i]\n\tRay:\n\t\tPos %f %f %f\n\t\tDir %f %f %f\n\tHit %s\n\t\tDist %f\n\t\tPos %f %f %f\n\t\tNormal %f %f %f\n\t\tColor %f %f %f %f\n\tPrev rayColor %f %f %f\n\tPrev light %f %f %f\n\n\n",i,ray.pos.x,ray.pos.y,ray.pos.z,ray.dir.x,ray.dir.y,ray.dir.z,hitInfo.didHit?"TRUE":"FALSE",hitInfo.dst,hitInfo.hitPoint.x,hitInfo.hitPoint.y,hitInfo.hitPoint.z,hitInfo.normal.x,hitInfo.normal.y,hitInfo.normal.z,hitInfo.mat.color.x,hitInfo.mat.color.y,hitInfo.mat.color.z,hitInfo.mat.emissionStrength,rayColor.x,rayColor.y,rayColor.z,incomingLight.x,incomingLight.y,incomingLight.z);
      if(hitInfo.didHit)
      {
         ray.dir = normalized(plus(hitInfo.normal,RandomDiretion()));//Cosine distribution //RandomHemisphereDirection(hitInfo.normal);
         ray.pos = plus(hitInfo.hitPoint,times(ray.dir,.01));

         vec3 emittedLight = times(hitInfo.mat.color,hitInfo.mat.emissionStrength);
         incomingLight = plus(incomingLight,timesVec3(emittedLight,rayColor));
         rayColor = timesVec3(rayColor,hitInfo.mat.color);
      }
      else
      {
         incomingLight = plus(incomingLight,timesVec3(getEnvironmentLight(ray),rayColor));
         break;
      }
   }
   return incomingLight;//vec3ToColor(incomingLight);
}

int main()
{
   normalizedSunDirection = normalized(sunDirection);
   Color image[w*h];
   vec3 origin={0,-1,-1};
   /*
   vec3 dir={0,0,1};
   dir=normalized(dir);
   Ray ray = {origin,dir};
   rngState = 0;
   calcColor(ray,3);
   */
   for(int y=0;y<h;++y)
   {
      for(int x=0;x<w;++x)
      {
         vec3 dir={(x-halfW)/(float)halfH,(y-halfH)/(double)halfH,1};//aspect ratio respected
         //dir = minus(dir,origin);//fixed camera dir
         dir = normalized(dir);
         Ray ray = {origin,dir};
         rngState = x+y*w;
         vec3 accumulatedColor = {0,0,0};
         for(int i=0;i<accumulationCount;++i)
            accumulatedColor = plus(accumulatedColor,times(calcColor(ray,maxBounce),1./accumulationCount));
         image[x+y*w]=vec3ToColor(accumulatedColor);
      }
   }
   stbi_write_bmp("out.bmp",w,h,3,(void const*)image);
   
   return 0;
}