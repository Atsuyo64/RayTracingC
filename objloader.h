#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

typedef struct OBJTriangle
{
    float pos[3][3];
    float color[3];
    float emission;
    float roughness;
} OBJTriangle;

/**
 * loadObj function
 * @param filename
 */
int loadObj(const char *filename, OBJTriangle **triangles);