#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "raytracing.h"
                                 //pos,radius,{color,emission}
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
static Sphere const spheres[] = {{{0,1,0},2.5,{{1,1,1},0,0}},};//{{0,5,0},1.5,{{1,1,1},10,0}},
static const int sphereCount = sizeof(spheres)/sizeof(Sphere);
static Triangle *triangles=NULL; //to parse from triangles.txt
static int triangleCount = 0;

static int const w = 128*2;//width
static int const h = 128*2;//height
static int const accumulationCount = 100*20;//1000 = ok
static int const maxBounce = 10;
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

#define EPSILON 0.001
