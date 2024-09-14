#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stbi_image_write.h"

typedef unsigned char uint8;  

typedef struct vec3
{
   float x,y,z;
} vec3;

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
   float smoothness;
} Material;

typedef struct Sphere
{
   vec3 pos;
   float r;
   Material mat;
} Sphere;

typedef struct Triangle
{
   vec3 posA,posB,posC,normal;
   Material mat;
} Triangle;

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
static Sphere const spheres[] = {{{2,-.8,2},1,{{1,1,1},0,1}},
                                 {{.51,-.3,2},.5,{{1,0,0},0,.75}},
                                 {{-1.5,.05,2},.4,{{0,1,0},0,.5}},
                                 {{-2.2,.1,2},.3,{{.4,.4,1},0,.25}},
                                 {{0,30.1,3},30,{{0.61,0,1},0,.75}},
                                 //{{-15,-15,10},15,{{1,.85,.85},1}},
                                 };
                                 //{{{0,0.7071068,4},1,{{1,1,1},1}},{{0,0,2},1,{{1,0,0},1}}};//debug
static const int sphereCount = sizeof(spheres)/sizeof(Sphere);
static Triangle *triangles=NULL; //to parse from triangles.txt
static int triangleCount = 0;

static int const w = 128*2;//width
static int const h = 128*2;//height
static int const accumulationCount = 100*1;//1000 = ok
static int const maxBounce = 2;
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
static vec3 WHITE = {1,1,1};
static vec3 BLACK = {0,0,0};
#define EPSILON 0.001

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

void cleanFile(char const* src,char const* dest)
{
   FILE* in = fopen(src,"r");
   if(in==NULL)return;
   FILE* out = fopen(dest,"w");
   if(out==NULL)return;
   char c,r;
   while((c=fgetc(in))!=EOF)
   {
      if(('0'<=c && c<='9') || c=='-' || c=='.' || c=='\n' || c=='+')
         fputc(c,out);
      else
         fputc(' ',out);
   }
   //fputc('\0',out);
   fclose(in);
   fclose(out);
}

void parseAndPlaceTriangle(Triangle* t,FILE* file)
{
   fscanf(file,"%f %f %f %f %f %f %f %f %f %f %f %f %f %f",&t->posA.x,&t->posA.y,
   &t->posA.z,&t->posB.x,&t->posB.y,&t->posB.z,&t->posC.x,&t->posC.y,&t->posC.z,
   &t->mat.color.x,&t->mat.color.y,&t->mat.color.z,&t->mat.emissionStrength,&t->mat.smoothness);
   t->normal = normalized(cross(minus(t->posB,t->posA),minus(t->posC,t->posA)));
}

void printTriangle(Triangle t)
{
   printf("Triangle:\n\tPos:\t(%f, %f, %f)\n\t\t(%f, %f, %f)\n\t\t(%f, %f, %f)\n\tNormale:\t(%f, %f, %f)\n\tMaterial:\n\t\tColor: (%f, %f, %f)\n\t\tEmission: %f\n\t\tSmoothness: %f\n\n",
   t.posA.x,t.posA.y,t.posA.z,t.posB.x,t.posB.y,t.posB.z,t.posC.x,t.posC.y,t.posC.z,t.normal.x,t.normal.y,t.normal.z,t.mat.color.x,t.mat.color.y,t.mat.color.z,t.mat.emissionStrength,t.mat.smoothness);
}

void printAllTriangles()
{
   for(int i=0;i<triangleCount;++i)
      printTriangle(triangles[i]);
}

void parseTriangleFile(char const* name)
{
   char nameParsed[256];
   strcpy(nameParsed,name);
   strcat(nameParsed,".parsed");
   cleanFile(name,nameParsed);
   FILE* file = fopen(nameParsed,"r");
   if(file==NULL)return;
   fscanf(file,"%i",&triangleCount);
   printf("%i triangles found\n",triangleCount);
   triangles=malloc(sizeof(Triangle)*triangleCount);
   printf("Loading...");
   fflush(stdin);
   for(int i=0;i<triangleCount;++i)
   {
      parseAndPlaceTriangle(triangles+i,file);
      printf(".");
      fflush(stdin);
   }
   printf("\n");
   fclose(file);
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
   HitInfo hitInfo = {0};
   vec3 offset = minus(ray.pos,sphereCentre);
   float b = dot(offset,ray.dir);
   float c = dot(offset,offset)-radius*radius;

   float delta = b*b-c;

   if(delta<0)
      return hitInfo;
   delta = sqrt(delta);
   float dst = -b-delta;
   if(dst<EPSILON)
      dst=-b+delta;
   if(dst<EPSILON)
      return hitInfo;
   hitInfo.didHit = 1;
   hitInfo.dst = dst;
   hitInfo.hitPoint = plus(ray.pos,times(ray.dir,dst));
   hitInfo.normal = normalized(minus(hitInfo.hitPoint,sphereCentre));
   return hitInfo;
}

HitInfo rayTriangle(Ray ray,Triangle t)
{
   HitInfo hitInfo = {0};
   vec3 AB = minus(t.posB,t.posA);
   vec3 AC = minus(t.posC,t.posA);
   vec3 h = cross(ray.dir,AC);
   float det = dot(AB,h);
   if(-EPSILON < det && det < EPSILON) return hitInfo;
   float invDet = 1./det;
   vec3 s = minus(ray.pos,t.posA);
   float u = dot(s,h) * invDet;
   if(u<0 || u>1)return hitInfo;
   vec3 q = cross(s,AB);
   float v = dot(ray.dir,q) * invDet;
   if(v<0 || u+v>1) return hitInfo;
   float dst = dot(AC,q) * invDet;
   if(dst<EPSILON) return hitInfo;
   hitInfo.dst = dst;
   //hitInfo.hitPoint = plus(ray.pos,times(ray.dir,dst));
   hitInfo.didHit = 1;
   hitInfo.normal = t.normal;
   return hitInfo;
}

HitInfo CalculateRayCollision(Ray ray)
{
   HitInfo closest = {0,999999};
   for(int i=0;i<sphereCount;++i)
   {
      HitInfo hitInfo = raySphere(ray,spheres[i].pos,spheres[i].r);
      if(hitInfo.didHit && hitInfo.dst<closest.dst)
      {
         closest=hitInfo;
         closest.mat = spheres[i].mat;
      }
   }
   for(int i=0;i<triangleCount;++i)
   {
      HitInfo hitInfo = rayTriangle(ray,triangles[i]);
      if(hitInfo.didHit && hitInfo.dst<closest.dst)
      {
         closest=hitInfo;
         closest.mat = triangles[i].mat;
      }
   }
   closest.hitPoint = plus(ray.pos,times(ray.dir,closest.dst));
   return closest;
}

vec3 calcDebugColor(Ray ray,int maxBounce)
{
   int i;
   for(i=0;i<maxBounce;++i)
   {
      HitInfo hitInfo = CalculateRayCollision(ray);
      //printf("[Bounce %i]\n\tRay:\n\t\tPos %f %f %f\n\t\tDir %f %f %f\n\tHit %s\n\t\tDist %f\n\t\tPos %f %f %f\n\t\tNormal %f %f %f\n\t\tColor %f %f %f %f\n\tPrev rayColor %f %f %f\n\tPrev light %f %f %f\n\n\n",i,ray.pos.x,ray.pos.y,ray.pos.z,ray.dir.x,ray.dir.y,ray.dir.z,hitInfo.didHit?"TRUE":"FALSE",hitInfo.dst,hitInfo.hitPoint.x,hitInfo.hitPoint.y,hitInfo.hitPoint.z,hitInfo.normal.x,hitInfo.normal.y,hitInfo.normal.z,hitInfo.mat.color.x,hitInfo.mat.color.y,hitInfo.mat.color.z,hitInfo.mat.emissionStrength,rayColor.x,rayColor.y,rayColor.z,incomingLight.x,incomingLight.y,incomingLight.z);
      if(hitInfo.didHit)
      {
         vec3 diffuseDir = normalized(plus(hitInfo.normal,RandomDiretion()));//Cosine distribution
         vec3 specularDir= reflect(ray.dir,hitInfo.normal);
         ray.dir = lerp(diffuseDir,specularDir,hitInfo.mat.smoothness);
         ray.pos = hitInfo.hitPoint;
      }
      else
         break;
   }
   return lerp(BLACK,WHITE,i/(float)maxBounce);
}

vec3 calcColor(Ray ray,int maxBounce)
{
   //Color skyColor={0x77,0xB5,0xFE};
   vec3 incomingLight = {0,0,0};
   vec3 rayColor = {1,1,1};
   int i;
   for(i=0;i<maxBounce;++i)
   {
      HitInfo hitInfo = CalculateRayCollision(ray);
      //printf("[Bounce %i]\n\tRay:\n\t\tPos %f %f %f\n\t\tDir %f %f %f\n\tHit %s\n\t\tDist %f\n\t\tPos %f %f %f\n\t\tNormal %f %f %f\n\t\tColor %f %f %f %f\n\tPrev rayColor %f %f %f\n\tPrev light %f %f %f\n\n\n",i,ray.pos.x,ray.pos.y,ray.pos.z,ray.dir.x,ray.dir.y,ray.dir.z,hitInfo.didHit?"TRUE":"FALSE",hitInfo.dst,hitInfo.hitPoint.x,hitInfo.hitPoint.y,hitInfo.hitPoint.z,hitInfo.normal.x,hitInfo.normal.y,hitInfo.normal.z,hitInfo.mat.color.x,hitInfo.mat.color.y,hitInfo.mat.color.z,hitInfo.mat.emissionStrength,rayColor.x,rayColor.y,rayColor.z,incomingLight.x,incomingLight.y,incomingLight.z);
      if(hitInfo.didHit)
      {
         vec3 diffuseDir = normalized(plus(hitInfo.normal,RandomDiretion()));//Cosine distribution //RandomHemisphereDirection(hitInfo.normal);
         vec3 specularDir= reflect(ray.dir,hitInfo.normal);
         ray.dir = lerp(diffuseDir,specularDir,hitInfo.mat.smoothness);
         ray.pos = hitInfo.hitPoint;//plus(hitInfo.hitPoint,times(ray.dir,.001));

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
   return incomingLight;
}

int main()
{
   normalizedSunDirection = normalized(sunDirection);
   printf("Parsing triangles...\n");
   parseTriangleFile("triangles.txt");
   //printAllTriangles();
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
         vec3 dir={(x-halfW)/(float)halfH,(y-halfH)/(float)halfH,1};//aspect ratio respected
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