#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "raytracing.h"
#include "scene.h"
#include "objloader.h"

/* COLORS & MATERIALS */

Color vec3ToColor(vec3 v)
{
   Color res = {floatToUint(v.x), floatToUint(v.y), floatToUint(v.z)};
   return res;
}

/* 3D OBJECTS STRUCTURES */

void parseAndPlaceTriangle(Triangle *t, FILE *file)
{
   fscanf(file, "%f %f %f %f %f %f %f %f %f %f %f %f %f %f", &t->posA.x, &t->posA.y,
          &t->posA.z, &t->posB.x, &t->posB.y, &t->posB.z, &t->posC.x, &t->posC.y, &t->posC.z,
          &t->mat.color.x, &t->mat.color.y, &t->mat.color.z, &t->mat.emissionStrength, &t->mat.smoothness);
   t->normal = normalized(cross(minus(t->posB, t->posA), minus(t->posC, t->posA)));
}

void printTriangle(Triangle t)
{
   printf("Triangle:\n\tPos:\t(%f, %f, %f)\n\t\t(%f, %f, %f)\n\t\t(%f, %f, %f)\n\tNormale:\t(%f, %f, %f)\n\tMaterial:\n\t\tColor: (%f, %f, %f)\n\t\tEmission: %f\n\t\tSmoothness: %f\n\n",
          t.posA.x, t.posA.y, t.posA.z, t.posB.x, t.posB.y, t.posB.z, t.posC.x, t.posC.y, t.posC.z, t.normal.x, t.normal.y, t.normal.z, t.mat.color.x, t.mat.color.y, t.mat.color.z, t.mat.emissionStrength, t.mat.smoothness);
}

void printAllTriangles()
{
   for (int i = 0; i < triangleCount; ++i)
      printTriangle(triangles[i]);
}

void skipUntilDelim(FILE *file, char delim)
{
   char c;
   do
      c = fgetc(file);
   while (c != delim);
}

void cleanFile(char const *src, char const *dest)
{
   FILE *in = fopen(src, "r");
   if (in == NULL)
      return;
   FILE *out = fopen(dest, "w");
   if (out == NULL)
      return;
   char c, r;
   while ((c = fgetc(in)) != EOF)
   {
      if (('0' <= c && c <= '9') || c == '-' || c == '.' || c == '\n' || c == '+')
         fputc(c, out);
      else if (c == '/') // allow comments in the file
      {
         c = fgetc(in);
         if (c == '/')
            skipUntilDelim(in, '\n');
         else
            ungetc(c, in);
      }
      else
         fputc(' ', out);
   }
   // fputc('\0',out);
   fclose(in);
   fclose(out);
}

void parseTriangleFile(char const *name)
{
   char nameParsed[256];
   strcpy(nameParsed, name);
   strcat(nameParsed, ".parsed");
   cleanFile(name, nameParsed);
   FILE *file = fopen(nameParsed, "r");
   if (file == NULL)
      return;
   fscanf(file, "%i", &triangleCount);
   printf("%i triangles found\n", triangleCount);
   triangles = malloc(sizeof(Triangle) * triangleCount);
   printf("Loading...");
   fflush(stdin);
   for (int i = 0; i < triangleCount; ++i)
   {
      parseAndPlaceTriangle(triangles + i, file);
      printf(".");
      fflush(stdin);
   }
   printf("\n");
   fclose(file);
}

void loadOBJTriangles(char const *filename)
{
   int ret;
   triangleCount = -1;
   OBJTriangle **objTriangles;
   ret = loadObj(filename, &objTriangles, &triangleCount);
   if (triangleCount == -1 || ret != 0)
   {
      fprintf(stderr, "ERROR WHILE LOADING OBJ ! (%s)", filename);
      exit(42);
   }
   triangles = malloc(sizeof(Triangle) * triangleCount);
   printf("Converting to triangles... 0/%d", triangleCount);
   fflush(stdin);
   for (int i = 0; i < triangleCount; ++i)
   {
      printf("\rConverting to RTC triangles... %d/%d", (i + 1), triangleCount);
      fflush(stdin);
      // rotateZ(180deg): -x -y z
      // coordinates

      // printf("0s %f\n", objTriangles[0]->smoothness);
      // printOBJTriangle(objTriangles[i]);
      triangles[i].posA.x = -objTriangles[i]->posA.x;
      triangles[i].posA.y = -objTriangles[i]->posA.y;
      triangles[i].posA.z = objTriangles[i]->posA.z;
      triangles[i].posB.x = -objTriangles[i]->posB.x;
      triangles[i].posB.y = -objTriangles[i]->posB.y;
      triangles[i].posB.z = objTriangles[i]->posB.z;
      triangles[i].posC.x = -objTriangles[i]->posC.x;
      triangles[i].posC.y = -objTriangles[i]->posC.y;
      triangles[i].posC.z = objTriangles[i]->posC.z;
      // normal
      triangles[i].normal.x = -objTriangles[i]->normal.x;
      triangles[i].normal.y = -objTriangles[i]->normal.y;
      triangles[i].normal.z = objTriangles[i]->normal.z;
      // shading
      triangles[i].mat.color.x = objTriangles[i]->color.x;
      triangles[i].mat.color.y = objTriangles[i]->color.y;
      triangles[i].mat.color.z = objTriangles[i]->color.z;
      triangles[i].mat.emissionStrength = objTriangles[i]->emission;
      triangles[i].mat.smoothness = objTriangles[i]->smoothness;
      // printTriangle(triangles[i]);
   }
   printf("\n");

   objTriangleArrayFree(objTriangles, triangleCount);
}

/* RAYS */

vec3 getEnvironmentLight(Ray ray, Scene s)
{
   float skyGradientT = powf(smoothstep(0, .74, -ray.dir.y), .35);
   vec3 skyGradient = lerp(s.skyColorHorizon, s.skyColorZenith, skyGradientT);
   float sun = powf(fmax(0, dot(ray.dir, s.normalizedSunDirection)), s.sunFocus) * s.sunIntensity;
   float groundToSkyT = smoothstep(-0.01, 0, -ray.dir.y);
   float sunMask = ray.dir.y < 0;
   vec3 sunValue = {sun * sunMask, sun * sunMask, sun * sunMask};
   return plus(lerp(s.groundColor, skyGradient, groundToSkyT), sunValue);
}

HitInfo raySphere(Ray ray, vec3 sphereCentre, float radius)
{
   HitInfo hitInfo = {0};
   vec3 offset = minus(ray.pos, sphereCentre);
   float b = dot(offset, ray.dir);
   float c = dot(offset, offset) - radius * radius;

   float delta = b * b - c;

   if (delta < 0)
      return hitInfo;
   delta = sqrt(delta);
   float dst = -b - delta;
   if (dst < EPSILON)
      dst = -b + delta;
   if (dst < EPSILON)
      return hitInfo;
   hitInfo.didHit = 1;
   hitInfo.dst = dst;
   hitInfo.hitPoint = plus(ray.pos, times(ray.dir, dst));
   hitInfo.normal = normalized(minus(hitInfo.hitPoint, sphereCentre));
   return hitInfo;
}

HitInfo rayTriangle(Ray ray, Triangle t)
{
   HitInfo hitInfo = {0};
   if (dot(ray.dir, t.normal) >= 0)
      return hitInfo;
   vec3 AB = minus(t.posB, t.posA);
   vec3 AC = minus(t.posC, t.posA);
   vec3 h = cross(ray.dir, AC);
   float det = dot(AB, h);
   if (-EPSILON < det && det < EPSILON)
      return hitInfo;
   float invDet = 1. / det;
   vec3 s = minus(ray.pos, t.posA);
   float u = dot(s, h) * invDet;
   if (u < 0 || u > 1)
      return hitInfo;
   vec3 q = cross(s, AB);
   float v = dot(ray.dir, q) * invDet;
   if (v < 0 || u + v > 1)
      return hitInfo;
   float dst = dot(AC, q) * invDet;
   if (dst < EPSILON)
      return hitInfo;
   hitInfo.dst = dst;
   // hitInfo.hitPoint = plus(ray.pos,times(ray.dir,dst));
   hitInfo.didHit = 1;
   hitInfo.normal = t.normal;
   return hitInfo;
}

HitInfo calculateRayCollision(Ray ray, int trianglesOnly)
{
   HitInfo closest = {0, 999999};
   if (trianglesOnly == 0)
      for (int i = 0; i < sphereCount; ++i)
      {
         HitInfo hitInfo = raySphere(ray, spheres[i].pos, spheres[i].r);
         if (hitInfo.didHit && hitInfo.dst < closest.dst)
         {
            closest = hitInfo;
            closest.mat = spheres[i].mat;
         }
      }
   for (int i = 0; i < triangleCount; ++i)
   {
      HitInfo hitInfo = rayTriangle(ray, triangles[i]);
      if (hitInfo.didHit && hitInfo.dst < closest.dst)
      {
         closest = hitInfo;
         closest.mat = triangles[i].mat;
      }
   }
   closest.hitPoint = plus(ray.pos, times(ray.dir, closest.dst));
   return closest;
}

vec3 calcDebugColor(Ray ray, int trianglesOnly, int maxBounce, Scene s)
{
   int i;
   for (i = 0; i < maxBounce; ++i)
   {
      HitInfo hitInfo = calculateRayCollision(ray, trianglesOnly);
      // printf("[Bounce %i]\n\tRay:\n\t\tPos %f %f %f\n\t\tDir %f %f %f\n\tHit %s\n\t\tDist %f\n\t\tPos %f %f %f\n\t\tNormal %f %f %f\n\t\tColor %f %f %f %f\n\tPrev rayColor %f %f %f\n\tPrev light %f %f %f\n\n\n",i,ray.pos.x,ray.pos.y,ray.pos.z,ray.dir.x,ray.dir.y,ray.dir.z,hitInfo.didHit?"TRUE":"FALSE",hitInfo.dst,hitInfo.hitPoint.x,hitInfo.hitPoint.y,hitInfo.hitPoint.z,hitInfo.normal.x,hitInfo.normal.y,hitInfo.normal.z,hitInfo.mat.color.x,hitInfo.mat.color.y,hitInfo.mat.color.z,hitInfo.mat.emissionStrength,rayColor.x,rayColor.y,rayColor.z,incomingLight.x,incomingLight.y,incomingLight.z);
      if (hitInfo.didHit)
      {
         vec3 diffuseDir = normalized(plus(hitInfo.normal, RandomDiretion())); // Cosine distribution
         vec3 specularDir = reflect(ray.dir, hitInfo.normal);
         ray.dir = lerp(diffuseDir, specularDir, hitInfo.mat.smoothness);
         ray.pos = hitInfo.hitPoint;
      }
      else
         break;
   }
   return lerp(BLACK, WHITE, i / (float)maxBounce);
}

vec3 calcColor(Ray ray, int trianglesOnly, int maxBounce, Scene s)
{
   // Color skyColor={0x77,0xB5,0xFE};
   vec3 incomingLight = {0, 0, 0};
   vec3 rayColor = {1, 1, 1};
   int i;
   for (i = 0; i < maxBounce; ++i)
   {
      HitInfo hitInfo = calculateRayCollision(ray, trianglesOnly);
      // printf("[Bounce %i]\n\tRay:\n\t\tPos %f %f %f\n\t\tDir %f %f %f\n\tHit %s\n\t\tDist %f\n\t\tPos %f %f %f\n\t\tNormal %f %f %f\n\t\tColor %f %f %f %f\n\tPrev rayColor %f %f %f\n\tPrev light %f %f %f\n\n\n",i,ray.pos.x,ray.pos.y,ray.pos.z,ray.dir.x,ray.dir.y,ray.dir.z,hitInfo.didHit?"TRUE":"FALSE",hitInfo.dst,hitInfo.hitPoint.x,hitInfo.hitPoint.y,hitInfo.hitPoint.z,hitInfo.normal.x,hitInfo.normal.y,hitInfo.normal.z,hitInfo.mat.color.x,hitInfo.mat.color.y,hitInfo.mat.color.z,hitInfo.mat.emissionStrength,rayColor.x,rayColor.y,rayColor.z,incomingLight.x,incomingLight.y,incomingLight.z);
      if (hitInfo.didHit)
      {
         vec3 diffuseDir = normalized(plus(hitInfo.normal, RandomDiretion())); // Cosine distribution //RandomHemisphereDirection(hitInfo.normal);
         vec3 specularDir = reflect(ray.dir, hitInfo.normal);
         ray.dir = lerp(diffuseDir, specularDir, hitInfo.mat.smoothness);
         ray.pos = hitInfo.hitPoint; // plus(hitInfo.hitPoint,times(ray.dir,.001));

         vec3 emittedLight = times(hitInfo.mat.color, hitInfo.mat.emissionStrength);
         incomingLight = plus(incomingLight, timesVec3(emittedLight, rayColor));
         rayColor = timesVec3(rayColor, hitInfo.mat.color);

         // optimisation that statistically does not change the result
         float p = fmax(fmax(rayColor.x, rayColor.y), rayColor.z);
         if (p < RandomValue())
            break;
         rayColor = times(rayColor, 1.0 / p);
      }
      else
      {
         incomingLight = plus(incomingLight, timesVec3(getEnvironmentLight(ray, s), rayColor));
         break;
      }
   }
   return incomingLight;
}
