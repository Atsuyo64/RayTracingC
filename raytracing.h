#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "moremath.h"

typedef struct Scene
{
    vec3 normalizedSunDirection, skyColorHorizon, skyColorZenith, groundColor;
    float sunFocus, sunIntensity;
} Scene;

/* COLORS & MATERIALS */

typedef struct Color
{
   uint8 r, g, b;
} Color;

static vec3 WHITE = {1, 1, 1};
static vec3 BLACK = {0, 0, 0};

Color vec3ToColor(vec3 v);

typedef struct Material
{
   vec3 color;
   float emissionStrength;
   float smoothness;
} Material;

/* 3D OBJECTS STRUCTURES */

typedef struct Sphere
{
   vec3 pos;
   float r;
   Material mat;
} Sphere;

typedef struct Triangle
{
   vec3 posA, posB, posC, normal;
   Material mat;
} Triangle;

void parseAndPlaceTriangle(Triangle *t, FILE *file);
void printTriangle(Triangle t);
void printAllTriangles();
void cleanFile(char const *src, char const *dest);
void parseTriangleFile(char const *name);
void loadOBJTriangles(char const *filename);

/* RAYS */

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

vec3 getEnvironmentLight(Ray ray, Scene s);
HitInfo raySphere(Ray ray, vec3 sphereCentre, float radius);
HitInfo rayTriangle(Ray ray, Triangle t);
HitInfo calculateRayCollision(Ray ray, int trianglesOnly);
vec3 calcDebugColor(Ray ray, int trianglesOnly, int maxBounce, Scene s);
vec3 calcColor(Ray ray, int trianglesOnly, int maxBounce, Scene s);