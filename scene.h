#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "raytracing.h"
// pos,radius,{color,emission}
/*
static Sphere const spheres[] = {{{2,-.8,2},1,{{1,1,1},0,1}},
                                 {{.51,-.3,2},.5,{{1,0,0},0,.75}},
                                 {{-1.5,.05,2},.4,{{0,1,0},0,.5}},
                                 {{-2.2,.1,2},.3,{{.4,.4,1},0,.25}},
                                 {{0,30.1,3},30,{{0.61,0,1},0,.75}},
                                 //{{-15,-15,10},15,{{1,.85,.85},1}},
                                 };
                                 //{{{0,0.7071068,4},1,{{1,1,1},1}},{{0,0,2},1,{{1,0,0},1}}};//debug
*/
static Sphere const spheres[] = {
    {{0, 1, 0}, 2.5, {{1, 1, 1}, 0, 0}},
}; //{{0,5,0},1.5,{{1,1,1},10,0}},
static const int sphereCount = sizeof(spheres) / sizeof(Sphere);
static Triangle *triangles = NULL; // to parse from triangles.txt
static int triangleCount = 0;

// static int const w = 128*1;//width // MOVED BACK TO MAIN.C
// static int const h = 128*1;//height // MOVED BACK TO MAIN.C
static int const accumulationCount = 100 * 40; // 1000 = ok
// static int const maxBounce = 10; // MOVED BACK TO MAIN.C
// static vec3 const sunDirection = {-30,-85,100}; // MOVED BACK TO MAIN.C
// static vec3 const SkyColorHorizon = {1,1,1}; // MOVED BACK TO MAIN.C
// static vec3 const SkyColorZenith = {0.263,0.969,0.871};//{.5,.5,1}; // MOVED BACK TO MAIN.C
// static vec3 const GroundColor = {.66,.66,.66}; // MOVED BACK TO MAIN.C
// static float const SunFocus = 22; // MOVED BACK TO MAIN.C
// static float const SunIntensity = .75; // MOVED BACK TO MAIN.C

static unsigned int rngState;

#define EPSILON 0.001