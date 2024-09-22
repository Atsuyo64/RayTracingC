#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef unsigned char uint8;
uint8 floatToUint(float f);


typedef struct vec3
{
   float x,y,z;
} vec3;


float length(vec3 v);
vec3 normalized(vec3 v);
vec3 mkV3(float x, float y, float z);

vec3 plus(vec3 a,vec3 b);
vec3 minus(vec3 a,vec3 b);
vec3 times(vec3 a,float b);
vec3 timesVec3(vec3 a,vec3 b);

float dot(vec3 a,vec3 b);
/**
 * Right hand cross product
 */
vec3 cross(vec3 u,vec3 v);
vec3 reflect(vec3 dir,vec3 normal);

float clamp(float x);
float smoothstep(float inf,float sup,float x);
/**
 * Linear interpolation
 */
vec3 lerp(vec3 inf,vec3 sup,float t);

/* RANDOM VALUES */

/**
 * /!\ using rngState variable
 */
float RandomValue();
float RandomValueNormalDistrubtion();
vec3 RandomDiretion();
vec3 RandomHemisphereDirection(vec3 normal);